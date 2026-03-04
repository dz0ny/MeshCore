#pragma once

#include <helpers/ui/UIScreen.h>
#include <helpers/ui/DisplayDriver.h>

class UITask;
class SensorManager;
class NodePrefs;

class SettingsScreen : public UIScreen {
public:
  SettingsScreen(UITask* task, SensorManager* sensors, NodePrefs* node_prefs);

  void reset();
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
  void poll() override;

private:
  enum SettingType {
    SETTING_BLE,
    SETTING_GPS,
    SETTING_BUZZER,
    SETTING_KEY_PRESS_BUZZER,
    SETTING_GPS_LOCADV_ENABLED,
    SETTING_GPS_LOCADV_DISTANCE,
    SETTING_GPS_LOCADV_FREQUENCY,
    SETTING_GPS_LOCADV_INTERVAL,
    SETTING_GPS_LOCADV_ACCURACY,
    SETTING_TELEMETRY,
    SETTING_ADVERTISE_LOCATION,
    SETTING_LANGUAGE,
    SETTING_CLEAR_FILES,
    SETTING_HIBERNATE
  };

  struct DisplayItem {
    bool is_header;
    SettingType setting_type;
    int setting_index;  // -1 for headers
  };

  UITask* _task;
  SensorManager* _sensors;
  NodePrefs* _node_prefs;

  int _selected_setting;
  int _scroll_offset;

  // Cached values for performance
  int _cached_num_settings;
  bool _cached_has_gps;
  int _cached_line_height;
  int _cached_line_spacing;

  // Reusable buffers (to avoid TR() buffer conflicts)
  char _display_buffer[64];
  char _key_buffer[40];
  char _value_buffer[24];

  // Helper methods
  int getNumSettings();
  void buildDisplayList(DisplayItem* items, int& item_count);
  void formatSettingValue(SettingType type, char* buf, size_t buf_size);
  void handleSettingToggle(SettingType type);
  void handleSettingChange(SettingType type, bool increase);
  const char* getIntervalString(int interval_code);
  template<typename T, size_t N>
  int cycleArrayValue(T& current, const T (&values)[N], bool forward = true);
};
