#include "SettingsScreen.h"
#include "UITask.h"
#include "UIHelpers.h"
#include "../MyMesh.h"
#include "helpers/ui/UIStrings.h"

extern MyMesh the_mesh;

static constexpr int SCREEN_TOP_MARGIN = 20;
static constexpr int SCREEN_BOTTOM_MARGIN = 5;

// Optimized macro to draw thin dotted border around selected setting
#define DRAW_SELECTION_BORDER(is_selected, y_pos, spacing) \
  if (is_selected) { \
    display.setColor(DisplayDriver::LIGHT); \
    drawDottedRect(display, 1, y_pos, display.width() - 2, (spacing) + 2, 3); \
    display.setColor(DisplayDriver::GREEN); \
  }

// Macro to draw section header (yellow with *)
#define DRAW_SECTION_HEADER(text, y_pos, spacing) \
  display.setColor(DisplayDriver::YELLOW); \
  display.setTextSize(1); \
  display.drawTextLeftAlign(2, y_pos, text); \
  display.setColor(DisplayDriver::GREEN); \
  y_pos += spacing;

// Helper function to draw a setting item
static void drawSetting(DisplayDriver& display, int& y, int line_spacing, bool is_selected, const char* text) {
  DRAW_SELECTION_BORDER(is_selected, y, line_spacing);
  display.drawTextLeftAlign(5, y, text);
  y += line_spacing;
}

SettingsScreen::SettingsScreen(UITask* task, SensorManager* sensors, NodePrefs* node_prefs)
  : _task(task),
    _sensors(sensors),
    _node_prefs(node_prefs),
    _selected_setting(0),
    _scroll_offset(0),
    _cached_num_settings(-1),
    _cached_has_gps(false),
    _cached_line_height(0),
    _cached_line_spacing(0) {
}

void SettingsScreen::reset() {
  _selected_setting = 0;
  _scroll_offset = 0;
  _cached_num_settings = -1;  // Force recalculation
}

int SettingsScreen::getNumSettings() {
  bool has_gps = false;
#if ENV_INCLUDE_GPS == 1
  has_gps = (_sensors->getLocationProvider() != NULL);
#endif

  // Recalculate if GPS state changed or not yet calculated
  if (_cached_num_settings < 0 || _cached_has_gps != has_gps) {
    _cached_has_gps = has_gps;
    _cached_num_settings = 10; // Base: BLE, Buzzer, Key Press Buzzer, Telemetry, Advert Loc, Packet Fwd, Language, Clear Files, Hibernate

    if (has_gps) {
      _cached_num_settings += 6; // GPS Toggle + 5 GPS LocAdv settings
    }
  }

  return _cached_num_settings;
}

const char* SettingsScreen::getIntervalString(int interval_code) {
  switch (interval_code) {
    case 0: return TR(STR_INTERVAL_1M);
    case 1: return TR(STR_INTERVAL_2M);
    case 2: return TR(STR_INTERVAL_5M);
    case 3: return TR(STR_INTERVAL_10M);
    case 4: return TR(STR_INTERVAL_15M);
    default: return "?";
  }
}

template<typename T, size_t N>
int SettingsScreen::cycleArrayValue(T& current, const T (&values)[N], bool forward) {
  for (size_t i = 0; i < N; i++) {
    if (current == values[i]) {
      if (forward) {
        current = values[(i + 1) % N];
      } else {
        current = values[(i + N - 1) % N];
      }
      return current;
    }
  }
  current = values[0];
  return current;
}

void SettingsScreen::buildDisplayList(DisplayItem* items, int& item_count) {
  item_count = 0;
  int setting_idx = 0;

  // Connectivity Header
  items[item_count++] = {true, SETTING_BLE, -1};

  // BLE
  items[item_count++] = {false, SETTING_BLE, setting_idx++};

#if ENV_INCLUDE_GPS == 1
  if (_cached_has_gps) {
    // GPS Toggle
    items[item_count++] = {false, SETTING_GPS, setting_idx++};
  }
#endif

  // Sound Header
  items[item_count++] = {true, SETTING_BUZZER, -1};

  // Buzzer
  items[item_count++] = {false, SETTING_BUZZER, setting_idx++};

  // Key Press Buzzer
  items[item_count++] = {false, SETTING_KEY_PRESS_BUZZER, setting_idx++};

#if ENV_INCLUDE_GPS == 1
  if (_cached_has_gps) {
    // Location Advertising Header
    items[item_count++] = {true, SETTING_GPS_LOCADV_ENABLED, -1};

    // GPS LocAdv settings
    items[item_count++] = {false, SETTING_GPS_LOCADV_ENABLED, setting_idx++};
    items[item_count++] = {false, SETTING_GPS_LOCADV_DISTANCE, setting_idx++};
    items[item_count++] = {false, SETTING_GPS_LOCADV_FREQUENCY, setting_idx++};
    items[item_count++] = {false, SETTING_GPS_LOCADV_INTERVAL, setting_idx++};
    items[item_count++] = {false, SETTING_GPS_LOCADV_ACCURACY, setting_idx++};
  }
#endif

  // Privacy Header
  items[item_count++] = {true, SETTING_TELEMETRY, -1};

  // Telemetry
  items[item_count++] = {false, SETTING_TELEMETRY, setting_idx++};

  // Advertise Location
  items[item_count++] = {false, SETTING_ADVERTISE_LOCATION, setting_idx++};

  // Maintenance Header
  items[item_count++] = {true, SETTING_LANGUAGE, -1};

  // Language
  items[item_count++] = {false, SETTING_LANGUAGE, setting_idx++};

  // Clear Files
  items[item_count++] = {false, SETTING_CLEAR_FILES, setting_idx++};

  // Hibernate
  items[item_count++] = {false, SETTING_HIBERNATE, setting_idx++};
}

void SettingsScreen::formatSettingValue(SettingType type, char* buf, size_t buf_size) {
  // Use member buffers to avoid TR() conflicts (TR returns pointer to static buffer)
  switch (type) {
    case SETTING_BLE:
      strncpy(_key_buffer, TR(STR_BLUETOOTH), sizeof(_key_buffer) - 1);
      strncpy(_value_buffer, _task->isSerialEnabled() ? TR(STR_ON) : TR(STR_OFF), sizeof(_value_buffer) - 1);
      snprintf(buf, buf_size, "%s: %s", _key_buffer, _value_buffer);
      break;

    case SETTING_GPS:
      strncpy(_key_buffer, TR(STR_GPS), sizeof(_key_buffer) - 1);
      strncpy(_value_buffer, _task->getGPSState() ? TR(STR_ON) : TR(STR_OFF), sizeof(_value_buffer) - 1);
      snprintf(buf, buf_size, "%s: %s", _key_buffer, _value_buffer);
      break;

    case SETTING_BUZZER:
#ifdef PIN_BUZZER
      strncpy(_key_buffer, TR(STR_BUZZER), sizeof(_key_buffer) - 1);
      strncpy(_value_buffer, _task->getBuzzerState() ? TR(STR_ON) : TR(STR_OFF), sizeof(_value_buffer) - 1);
#else
      strncpy(_key_buffer, TR(STR_BUZZER), sizeof(_key_buffer) - 1);
      strncpy(_value_buffer, TR(STR_NOT_AVAILABLE), sizeof(_value_buffer) - 1);
#endif
      snprintf(buf, buf_size, "%s: %s", _key_buffer, _value_buffer);
      break;

    case SETTING_KEY_PRESS_BUZZER:
#ifdef PIN_BUZZER
      strncpy(_key_buffer, TR(STR_KEY_PRESS_BUZZER), sizeof(_key_buffer) - 1);
      strncpy(_value_buffer, _node_prefs->buzzer_key_press ? TR(STR_ON) : TR(STR_OFF), sizeof(_value_buffer) - 1);
#else
      strncpy(_key_buffer, TR(STR_KEY_PRESS_BUZZER), sizeof(_key_buffer) - 1);
      strncpy(_value_buffer, TR(STR_NOT_AVAILABLE), sizeof(_value_buffer) - 1);
#endif
      snprintf(buf, buf_size, "%s: %s", _key_buffer, _value_buffer);
      break;

    case SETTING_GPS_LOCADV_ENABLED:
      strncpy(_key_buffer, TR(STR_BROADCAST_LOCATION), sizeof(_key_buffer) - 1);
      strncpy(_value_buffer, _node_prefs->gps_loc_advert_enabled ? TR(STR_ON) : TR(STR_OFF), sizeof(_value_buffer) - 1);
      snprintf(buf, buf_size, "%s: %s", _key_buffer, _value_buffer);
      break;

    case SETTING_GPS_LOCADV_DISTANCE:
      strncpy(_key_buffer, TR(STR_MOVEMENT_THRESHOLD), sizeof(_key_buffer) - 1);
      snprintf(buf, buf_size, "%s: %dm", _key_buffer, _node_prefs->gps_loc_distance_threshold);
      break;

    case SETTING_GPS_LOCADV_FREQUENCY:
      strncpy(_key_buffer, TR(STR_UPDATE_FREQUENCY), sizeof(_key_buffer) - 1);
      snprintf(buf, buf_size, "%s: %ds", _key_buffer, _node_prefs->gps_loc_frequency * 10);
      break;

    case SETTING_GPS_LOCADV_INTERVAL:
      strncpy(_key_buffer, TR(STR_GUARANTEED_INTERVAL), sizeof(_key_buffer) - 1);
      strncpy(_value_buffer, getIntervalString(_node_prefs->gps_loc_guaranteed_interval), sizeof(_value_buffer) - 1);
      snprintf(buf, buf_size, "%s: %s", _key_buffer, _value_buffer);
      break;

    case SETTING_GPS_LOCADV_ACCURACY:
      strncpy(_key_buffer, TR(STR_REQUIRED_ACCURACY), sizeof(_key_buffer) - 1);
      snprintf(buf, buf_size, "%s: %dm", _key_buffer, _node_prefs->gps_loc_accuracy_threshold);
      break;

    case SETTING_TELEMETRY:
      strncpy(_key_buffer, TR(STR_TELEMETRY_SHARE), sizeof(_key_buffer) - 1);
      if (_node_prefs->telemetry_mode_base == TELEM_MODE_ALLOW_ALL) {
        strncpy(_value_buffer, TR(STR_ALL), sizeof(_value_buffer) - 1);
      } else if (_node_prefs->telemetry_mode_base == TELEM_MODE_ALLOW_FLAGS) {
        strncpy(_value_buffer, TR(STR_FLAGS), sizeof(_value_buffer) - 1);
      } else {
        strncpy(_value_buffer, TR(STR_DENY), sizeof(_value_buffer) - 1);
      }
      snprintf(buf, buf_size, "%s: %s", _key_buffer, _value_buffer);
      break;

    case SETTING_ADVERTISE_LOCATION:
      strncpy(_key_buffer, TR(STR_ADVERTISE_LOCATION), sizeof(_key_buffer) - 1);
      strncpy(_value_buffer, (_node_prefs->advert_loc_policy == ADVERT_LOC_SHARE) ? TR(STR_ON) : TR(STR_OFF), sizeof(_value_buffer) - 1);
      snprintf(buf, buf_size, "%s: %s", _key_buffer, _value_buffer);
      break;

    case SETTING_LANGUAGE:
      strncpy(_key_buffer, TR(STR_LANGUAGE), sizeof(_key_buffer) - 1);
      strncpy(_value_buffer, uiGetLanguageName(_node_prefs, uiGetCurrentLanguage(_node_prefs)), sizeof(_value_buffer) - 1);
      snprintf(buf, buf_size, "%s: %s", _key_buffer, _value_buffer);
      break;

    case SETTING_CLEAR_FILES:
      strncpy(buf, TR(STR_CLEAR_FILES), buf_size - 1);
      break;

    case SETTING_HIBERNATE:
      strncpy(buf, TR(STR_HIBERNATE), buf_size - 1);
      break;
  }
}

int SettingsScreen::render(DisplayDriver& display) {
  display.clear();
  display.startFrame();

  // Update cached settings count
  int num_settings = getNumSettings();

  // Draw header
  DRAW_SCREEN_HEADER(TR(STR_SETTINGS), _task);

  display.setColor(DisplayDriver::GREEN);
  display.setTextSize(1);

  // Cache line height if not already cached
  if (_cached_line_height == 0) {
    _cached_line_height = display.getTextHeight("A");
    _cached_line_spacing = _cached_line_height + 6;
  }

  // Build display list (includes headers and settings)
  DisplayItem display_list[30];  // Max possible items
  int total_items = 0;
  buildDisplayList(display_list, total_items);

  // Find selected item index in display list
  int selected_item_index = 0;
  for (int i = 0; i < total_items; i++) {
    if (!display_list[i].is_header && display_list[i].setting_index == _selected_setting) {
      selected_item_index = i;
      break;
    }
  }

  // Calculate visible items
  int available_height = display.height() - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN;
  int visible_items = max(2, min(available_height / _cached_line_spacing, total_items));

  // Calculate scroll offset
  if (selected_item_index < _scroll_offset) {
    _scroll_offset = selected_item_index;
  } else if (selected_item_index >= _scroll_offset + visible_items) {
    _scroll_offset = selected_item_index - visible_items + 1;
  }

  // Render visible items
  int y = SCREEN_TOP_MARGIN + (_cached_line_height / 2);

  for (int i = _scroll_offset; i < _scroll_offset + visible_items && i < total_items; i++) {
    const DisplayItem& item = display_list[i];

    if (item.is_header) {
      // Draw header
      const char* header_text;
      switch (item.setting_type) {
        case SETTING_BLE:
          header_text = TR(STR_HEADER_CONNECTIVITY);
          break;
        case SETTING_BUZZER:
          header_text = TR(STR_HEADER_SOUND);
          break;
        case SETTING_GPS_LOCADV_ENABLED:
          header_text = TR(STR_HEADER_LOCATION_ADVERT);
          break;
        case SETTING_TELEMETRY:
          header_text = TR(STR_HEADER_PRIVACY);
          break;
        case SETTING_LANGUAGE:
          header_text = TR(STR_HEADER_MAINTENANCE);
          break;
        default:
          header_text = "???";
          break;
      }
      DRAW_SECTION_HEADER(header_text, y, _cached_line_spacing);
    } else {
      // Draw setting
      formatSettingValue(item.setting_type, _display_buffer, sizeof(_display_buffer));
      drawSetting(display, y, _cached_line_spacing,
                  item.setting_index == _selected_setting, _display_buffer);
    }
  }

  // Draw scroll indicators
  int indicator_x = display.width() - 8;
  if (_scroll_offset > 0) {
    display.setColor(DisplayDriver::LIGHT);
    display.drawTextRightAlign(indicator_x, SCREEN_TOP_MARGIN, "^");
  }
  if (_scroll_offset + visible_items < total_items) {
    display.setColor(DisplayDriver::LIGHT);
    display.drawTextRightAlign(indicator_x, display.height() - SCREEN_BOTTOM_MARGIN, "v");
  }

  display.endFrame();
  return 1000;
}

void SettingsScreen::handleSettingToggle(SettingType type) {
  // Reuse member buffers for alerts
  switch (type) {
    case SETTING_BLE:
      if (_task->isSerialEnabled()) {
        _task->disableSerial();
        _task->notify(UIEventType::ack);
      } else {
        _task->enableSerial();
        _task->notify(UIEventType::ack);
      }
      break;

    case SETTING_GPS:
      _task->toggleGPS();
      break;

    case SETTING_BUZZER:
      _task->toggleBuzzer();
      break;

    case SETTING_KEY_PRESS_BUZZER:
#ifdef PIN_BUZZER
      _node_prefs->buzzer_key_press = !_node_prefs->buzzer_key_press;
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
#endif
      break;

    case SETTING_GPS_LOCADV_ENABLED:
      _node_prefs->gps_loc_advert_enabled = !_node_prefs->gps_loc_advert_enabled;
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
      break;

    case SETTING_GPS_LOCADV_DISTANCE: {
      uint8_t distances[] = {5, 10, 50, 100, 200, 250};  // Max 250m (uint8_t limit)
      cycleArrayValue(_node_prefs->gps_loc_distance_threshold, distances);
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
      break;
    }

    case SETTING_GPS_LOCADV_FREQUENCY: {
      uint8_t frequencies[] = {3, 6, 12, 30};
      cycleArrayValue(_node_prefs->gps_loc_frequency, frequencies);
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
      break;
    }

    case SETTING_GPS_LOCADV_INTERVAL:
      _node_prefs->gps_loc_guaranteed_interval = (_node_prefs->gps_loc_guaranteed_interval + 1) % 5;
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
      break;

    case SETTING_GPS_LOCADV_ACCURACY: {
      uint8_t accuracies[] = {5, 10, 20, 50, 100};
      cycleArrayValue(_node_prefs->gps_loc_accuracy_threshold, accuracies);
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
      break;
    }

    case SETTING_TELEMETRY:
      if (_node_prefs->telemetry_mode_base == TELEM_MODE_DENY) {
        _node_prefs->telemetry_mode_base = TELEM_MODE_ALLOW_FLAGS;
      } else if (_node_prefs->telemetry_mode_base == TELEM_MODE_ALLOW_FLAGS) {
        _node_prefs->telemetry_mode_base = TELEM_MODE_ALLOW_ALL;
      } else {
        _node_prefs->telemetry_mode_base = TELEM_MODE_DENY;
      }
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
      break;

    case SETTING_ADVERTISE_LOCATION:
      _node_prefs->advert_loc_policy = (_node_prefs->advert_loc_policy == ADVERT_LOC_SHARE) ?
                                        ADVERT_LOC_NONE : ADVERT_LOC_SHARE;
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
      break;

    case SETTING_LANGUAGE: {
      UILanguage current_lang = uiGetCurrentLanguage(_node_prefs);
      UILanguage next_lang = (UILanguage)((current_lang + 1) % LANG_COUNT);
      uiSetLanguage(_node_prefs, next_lang);
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
      break;
    }

    case SETTING_CLEAR_FILES:
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
      strncpy(_display_buffer, TR(STR_ALERT_CLEARING_FILES), sizeof(_display_buffer) - 1);
      _task->showAlert(_display_buffer, 1500);
      if (the_mesh.clearAllFilesExceptSettings()) {
        strncpy(_display_buffer, TR(STR_ALERT_FILES_CLEARED_REBOOTING), sizeof(_display_buffer) - 1);
        _task->showAlert(_display_buffer, 2000);
        delay(2000);
        _task->shutdown(true);
      } else {
        strncpy(_display_buffer, TR(STR_ALERT_CLEAR_FAILED), sizeof(_display_buffer) - 1);
        _task->showAlert(_display_buffer, 1500);
      }
      break;

    case SETTING_HIBERNATE:
      _task->notify(UIEventType::ack);
      strncpy(_display_buffer, TR(STR_HIBERNATING), sizeof(_display_buffer) - 1);
      _task->showAlert(_display_buffer, 1000);
      delay(1000);
      _task->shutdown(false);
      break;
  }
}

void SettingsScreen::handleSettingChange(SettingType type, bool increase) {
  switch (type) {
    case SETTING_BLE:
    case SETTING_GPS:
    case SETTING_BUZZER:
    case SETTING_KEY_PRESS_BUZZER:
    case SETTING_GPS_LOCADV_ENABLED:
    case SETTING_ADVERTISE_LOCATION:
      // Toggle settings - left/right both toggle
      handleSettingToggle(type);
      break;

    case SETTING_GPS_LOCADV_DISTANCE: {
      uint8_t distances[] = {5, 10, 50, 100, 200, 250};
      cycleArrayValue(_node_prefs->gps_loc_distance_threshold, distances, increase);
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
      break;
    }

    case SETTING_GPS_LOCADV_FREQUENCY: {
      uint8_t frequencies[] = {3, 6, 12, 30};
      cycleArrayValue(_node_prefs->gps_loc_frequency, frequencies, increase);
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
      break;
    }

    case SETTING_GPS_LOCADV_INTERVAL:
      if (increase) {
        _node_prefs->gps_loc_guaranteed_interval = (_node_prefs->gps_loc_guaranteed_interval + 1) % 5;
      } else {
        _node_prefs->gps_loc_guaranteed_interval = (_node_prefs->gps_loc_guaranteed_interval + 4) % 5;
      }
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
      break;

    case SETTING_GPS_LOCADV_ACCURACY: {
      uint8_t accuracies[] = {5, 10, 20, 50, 100};
      cycleArrayValue(_node_prefs->gps_loc_accuracy_threshold, accuracies, increase);
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
      break;
    }

    case SETTING_TELEMETRY:
      // Cycle through telemetry modes
      if (increase) {
        if (_node_prefs->telemetry_mode_base == TELEM_MODE_DENY) {
          _node_prefs->telemetry_mode_base = TELEM_MODE_ALLOW_FLAGS;
        } else if (_node_prefs->telemetry_mode_base == TELEM_MODE_ALLOW_FLAGS) {
          _node_prefs->telemetry_mode_base = TELEM_MODE_ALLOW_ALL;
        } else {
          _node_prefs->telemetry_mode_base = TELEM_MODE_DENY;
        }
      } else {
        if (_node_prefs->telemetry_mode_base == TELEM_MODE_DENY) {
          _node_prefs->telemetry_mode_base = TELEM_MODE_ALLOW_ALL;
        } else if (_node_prefs->telemetry_mode_base == TELEM_MODE_ALLOW_ALL) {
          _node_prefs->telemetry_mode_base = TELEM_MODE_ALLOW_FLAGS;
        } else {
          _node_prefs->telemetry_mode_base = TELEM_MODE_DENY;
        }
      }
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
      break;

    case SETTING_LANGUAGE: {
      UILanguage current_lang = uiGetCurrentLanguage(_node_prefs);
      UILanguage next_lang;
      if (increase) {
        next_lang = (UILanguage)((current_lang + 1) % LANG_COUNT);
      } else {
        next_lang = (UILanguage)((current_lang + LANG_COUNT - 1) % LANG_COUNT);
      }
      uiSetLanguage(_node_prefs, next_lang);
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
      break;
    }

    case SETTING_CLEAR_FILES:
    case SETTING_HIBERNATE:
      // Action items - cannot be changed with left/right
      break;
  }
}

bool SettingsScreen::handleInput(char c) {
  int num_settings = getNumSettings();

  if (c == KEY_UP || c == KEY_PREV) {
    if (_selected_setting > 0) {
      _selected_setting--;
    } else {
      _selected_setting = num_settings - 1;
    }
    return true;
  }

  if (c == KEY_DOWN || c == KEY_NEXT) {
    _selected_setting = (_selected_setting + 1) % num_settings;
    return true;
  }

  if (c == KEY_CANCEL) {
    _task->gotoHomeScreen();
    return true;
  }

  if (c == KEY_LEFT || c == KEY_RIGHT) {
    // Build display list to find setting type
    DisplayItem display_list[30];
    int total_items = 0;
    buildDisplayList(display_list, total_items);

    // Find selected setting
    for (int i = 0; i < total_items; i++) {
      if (!display_list[i].is_header && display_list[i].setting_index == _selected_setting) {
        handleSettingChange(display_list[i].setting_type, c == KEY_RIGHT);
        return true;
      }
    }
  }

  if (c == KEY_ENTER || c == KEY_SELECT) {
    // Build display list to find setting type
    DisplayItem display_list[30];
    int total_items = 0;
    buildDisplayList(display_list, total_items);

    // Find selected setting
    for (int i = 0; i < total_items; i++) {
      if (!display_list[i].is_header && display_list[i].setting_index == _selected_setting) {
        handleSettingToggle(display_list[i].setting_type);
        return true;
      }
    }
  }

  return false;
}

void SettingsScreen::poll() {
  // Nothing to poll for settings screen
}
