#include "SettingsScreen.h"
#include "UITask.h"
#include "UIHelpers.h"
#include "../MyMesh.h"

extern MyMesh the_mesh;

static constexpr int SCREEN_TOP_MARGIN = 20;
static constexpr int SCREEN_BOTTOM_MARGIN = 5;

// Macro to draw thin dotted border around selected setting
#define DRAW_SELECTION_BORDER(is_selected, y_pos, spacing) \
  if (is_selected) { \
    display.setColor(DisplayDriver::LIGHT); \
    int border_x = 1, border_y = (y_pos), border_w = display.width() - 2, border_h = (spacing) + 2; \
    for (int x = border_x; x < border_x + border_w; x += 3) { \
      display.fillRect(x, border_y, 1, 1); \
    } \
    for (int x = border_x; x < border_x + border_w; x += 3) { \
      display.fillRect(x, border_y + border_h - 1, 1, 1); \
    } \
    for (int y = border_y; y < border_y + border_h; y += 3) { \
      display.fillRect(border_x, y, 1, 1); \
    } \
    for (int y = border_y; y < border_y + border_h; y += 3) { \
      display.fillRect(border_x + border_w - 1, y, 1, 1); \
    } \
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
    _scroll_offset(0) {
}

void SettingsScreen::reset() {
  _selected_setting = 0;
  _scroll_offset = 0;
}

int SettingsScreen::render(DisplayDriver& display) {
  display.clear();
  display.startFrame();

  // Draw header
  DRAW_SCREEN_HEADER("Settings", _task);

  display.setColor(DisplayDriver::GREEN);
  display.setTextSize(1);
  int line_height = display.getTextHeight("A");
  int line_spacing = line_height + 6;

  // Count available settings (including headers as non-selectable items)
  int num_settings = 6;  // BLE, Buzzer, Key Press Buzzer, Telemetry, Advert Loc, Clear Files
  int num_headers = 4;   // Connectivity, Sound, Privacy, Maintenance
#if ENV_INCLUDE_GPS == 1
  if (_sensors->getLocationProvider() != NULL) {
    num_settings += 1; // GPS toggle
    num_settings += 5; // GPS LocAdv: enabled, distance, frequency, interval, accuracy
    num_headers += 2;  // GPS header + Location Advertising header
  }
#endif
  int total_items = num_settings + num_headers;

  // Calculate how many items can fit based on available height
  int available_height = display.height() - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN;
  int calculated_visible_items = available_height / line_spacing;

  // Ensure at least 2 items are visible, but cap at total items (settings + headers)
  int visible_items = max(2, min(calculated_visible_items, total_items));

  // Map selected_setting to its display item index (accounting for headers)
  int selected_item_index = 0;
  int setting_counter = 0;

  // Count items until we reach the selected setting
  selected_item_index++; // Connectivity header
  if (setting_counter == _selected_setting) goto found_selected;
  selected_item_index++; // BLE
  setting_counter++;

  selected_item_index++; // Sound header

  if (setting_counter == _selected_setting) goto found_selected;
  selected_item_index++; // Buzzer
  setting_counter++;

  if (setting_counter == _selected_setting) goto found_selected;
  selected_item_index++; // Key Press Buzzer
  setting_counter++;

#if ENV_INCLUDE_GPS == 1
  if (_sensors->getLocationProvider() != NULL) {
    selected_item_index++; // GPS header

    if (setting_counter == _selected_setting) goto found_selected;
    selected_item_index++; // GPS Toggle
    setting_counter++;

    selected_item_index++; // Location Advert header

    if (setting_counter == _selected_setting) goto found_selected;
    selected_item_index++; // LocAdv Enabled
    setting_counter++;

    if (setting_counter == _selected_setting) goto found_selected;
    selected_item_index++; // LocAdv Distance
    setting_counter++;

    if (setting_counter == _selected_setting) goto found_selected;
    selected_item_index++; // LocAdv Frequency
    setting_counter++;

    if (setting_counter == _selected_setting) goto found_selected;
    selected_item_index++; // LocAdv Interval
    setting_counter++;

    if (setting_counter == _selected_setting) goto found_selected;
    selected_item_index++; // LocAdv Accuracy
    setting_counter++;
  }
#endif

  selected_item_index++; // Privacy header

  if (setting_counter == _selected_setting) goto found_selected;
  selected_item_index++; // Telemetry
  setting_counter++;

  if (setting_counter == _selected_setting) goto found_selected;
  selected_item_index++; // Advertise Location
  setting_counter++;

  selected_item_index++; // Maintenance header

  if (setting_counter == _selected_setting) goto found_selected;
  selected_item_index++; // Clear Files
  setting_counter++;

found_selected:

  // Calculate scroll offset to keep selected item visible
  if (selected_item_index < _scroll_offset) {
    _scroll_offset = selected_item_index;
  } else if (selected_item_index >= _scroll_offset + visible_items) {
    _scroll_offset = selected_item_index - visible_items + 1;
  }

  // Start rendering from scroll offset, with top margin
  int y = SCREEN_TOP_MARGIN + (line_height / 2);
  char buf[50];
  int curr_item = 0;     // Index in display list (including headers)
  int curr_setting = 0;  // Index of actual settings (for selection)

  // === CONNECTIVITY HEADER ===
  if (curr_item >= _scroll_offset && curr_item < _scroll_offset + visible_items) {
    DRAW_SECTION_HEADER("* Connectivity", y, line_spacing);
  }
  curr_item++;

  // BLE setting
  if (curr_item >= _scroll_offset && curr_item < _scroll_offset + visible_items) {
    bool ble_on = _task->isSerialEnabled();
    sprintf(buf, "  Bluetooth: %s", ble_on ? "ON" : "OFF");
    drawSetting(display, y, line_spacing, curr_setting == _selected_setting, buf);
  }
  curr_item++;
  curr_setting++;

  // === SOUND HEADER ===
  if (curr_item >= _scroll_offset && curr_item < _scroll_offset + visible_items) {
    DRAW_SECTION_HEADER("* Sound", y, line_spacing);
  }
  curr_item++;

  // Buzzer setting
  if (curr_item >= _scroll_offset && curr_item < _scroll_offset + visible_items) {
#ifdef PIN_BUZZER
    bool buzzer_on = _task->getBuzzerState();
    sprintf(buf, "  Buzzer: %s", buzzer_on ? "ON" : "OFF");
#else
    sprintf(buf, "  Buzzer: N/A");
#endif
    drawSetting(display, y, line_spacing, curr_setting == _selected_setting, buf);
  }
  curr_item++;
  curr_setting++;

  // Key Press Buzzer setting
  if (curr_item >= _scroll_offset && curr_item < _scroll_offset + visible_items) {
#ifdef PIN_BUZZER
    bool key_press_on = _node_prefs->buzzer_key_press;
    sprintf(buf, "  Key Press Buzzer: %s",
            key_press_on ? "ON" : "OFF");
#else
    sprintf(buf, "  Key Press Buzzer: N/A");
#endif
    drawSetting(display, y, line_spacing, curr_setting == _selected_setting, buf);
  }
  curr_item++;
  curr_setting++;

#if ENV_INCLUDE_GPS == 1
  if (_sensors->getLocationProvider() != NULL) {
    // === GPS HEADER ===
    if (curr_item >= _scroll_offset && curr_item < _scroll_offset + visible_items) {
      DRAW_SECTION_HEADER("* GPS", y, line_spacing);
    }
    curr_item++;

    // GPS Toggle
    if (curr_item >= _scroll_offset && curr_item < _scroll_offset + visible_items) {
      bool gps_on = _task->getGPSState();
      sprintf(buf, "  GPS: %s", gps_on ? "ON" : "OFF");
      drawSetting(display, y, line_spacing, curr_setting == _selected_setting, buf);
    }
    curr_item++;
    curr_setting++;

    // === LOCATION ADVERTISING HEADER ===
    if (curr_item >= _scroll_offset && curr_item < _scroll_offset + visible_items) {
      DRAW_SECTION_HEADER("* Location Advert", y, line_spacing);
    }
    curr_item++;

    // GPS LocAdv Enabled
    if (curr_item >= _scroll_offset && curr_item < _scroll_offset + visible_items) {
      bool locadv_on = _node_prefs->gps_loc_advert_enabled;
      sprintf(buf, "  Broadcast Location: %s",
              locadv_on ? "ON" : "OFF");
      drawSetting(display, y, line_spacing, curr_setting == _selected_setting, buf);
    }
    curr_item++;
    curr_setting++;

    // GPS LocAdv Distance
    if (curr_item >= _scroll_offset && curr_item < _scroll_offset + visible_items) {
      sprintf(buf, "  Movement Threshold: %dm",
              _node_prefs->gps_loc_distance_threshold);
      drawSetting(display, y, line_spacing, curr_setting == _selected_setting, buf);
    }
    curr_item++;
    curr_setting++;

    // GPS LocAdv Frequency
    if (curr_item >= _scroll_offset && curr_item < _scroll_offset + visible_items) {
      sprintf(buf, "  Update Frequency: %ds",
              _node_prefs->gps_loc_frequency * 10);
      drawSetting(display, y, line_spacing, curr_setting == _selected_setting, buf);
    }
    curr_item++;
    curr_setting++;

    // GPS LocAdv Interval
    if (curr_item >= _scroll_offset && curr_item < _scroll_offset + visible_items) {
      const char* interval_str;
      switch (_node_prefs->gps_loc_guaranteed_interval) {
      case 0:
        interval_str = "1m";
        break;
      case 1:
        interval_str = "5m";
        break;
      case 2:
        interval_str = "15m";
        break;
      default:
        interval_str = "?";
        break;
      }
      sprintf(buf, "  Guaranteed Interval: %s", interval_str);
      drawSetting(display, y, line_spacing, curr_setting == _selected_setting, buf);
    }
    curr_item++;
    curr_setting++;

    // GPS LocAdv Accuracy
    if (curr_item >= _scroll_offset && curr_item < _scroll_offset + visible_items) {
      sprintf(buf, "  Required Accuracy: %dm",
              _node_prefs->gps_loc_accuracy_threshold);
      drawSetting(display, y, line_spacing, curr_setting == _selected_setting, buf);
    }
    curr_item++;
    curr_setting++;
  }
#endif

  // === PRIVACY HEADER ===
  if (curr_item >= _scroll_offset && curr_item < _scroll_offset + visible_items) {
    DRAW_SECTION_HEADER("* Privacy", y, line_spacing);
  }
  curr_item++;

  // Telemetry sharing setting
  if (curr_item >= _scroll_offset && curr_item < _scroll_offset + visible_items) {
    const char* telem_mode = "DENY";
    if (_node_prefs->telemetry_mode_base == TELEM_MODE_ALLOW_ALL) {
      telem_mode = "ALL";
    } else if (_node_prefs->telemetry_mode_base == TELEM_MODE_ALLOW_FLAGS) {
      telem_mode = "FLAGS";
    }
    sprintf(buf, "  Telemetry Share: %s", telem_mode);
    drawSetting(display, y, line_spacing, curr_setting == _selected_setting, buf);
  }
  curr_item++;
  curr_setting++;

  // Advertise location setting
  if (curr_item >= _scroll_offset && curr_item < _scroll_offset + visible_items) {
    bool advert_loc = (_node_prefs->advert_loc_policy == ADVERT_LOC_SHARE);
    sprintf(buf, "  Advertise Location: %s",
            advert_loc ? "ON" : "OFF");
    drawSetting(display, y, line_spacing, curr_setting == _selected_setting, buf);
  }
  curr_item++;
  curr_setting++;

  // === MAINTENANCE HEADER ===
  if (curr_item >= _scroll_offset && curr_item < _scroll_offset + visible_items) {
    DRAW_SECTION_HEADER("* Maintenance", y, line_spacing);
  }
  curr_item++;

  // Clear Files setting
  if (curr_item >= _scroll_offset && curr_item < _scroll_offset + visible_items) {
    sprintf(buf, "  Clear Files");
    drawSetting(display, y, line_spacing, curr_setting == _selected_setting, buf);
  }
  curr_item++;
  curr_setting++;

  // Draw scroll indicators on the right side
  int indicator_x = display.width() - 8;
  if (_scroll_offset > 0) {
    display.setColor(DisplayDriver::LIGHT);
    display.drawTextRightAlign(indicator_x, SCREEN_TOP_MARGIN, "^");
  }
  if (_scroll_offset + visible_items < total_items) {
    display.setColor(DisplayDriver::LIGHT);
    display.drawTextRightAlign(indicator_x, display.height() - SCREEN_BOTTOM_MARGIN, "v");
  }

  // Footer removed for cleaner UI

  return 1000; // Refresh every second
}

bool SettingsScreen::handleInput(char c) {
  // Count total settings
  int num_settings = 6; // BLE, Buzzer, Key Press Buzzer, Telemetry, Advert Loc, Clear Files
#if ENV_INCLUDE_GPS == 1
  if (_sensors->getLocationProvider() != NULL) {
    num_settings += 1; // GPS toggle
    num_settings += 5; // GPS LocAdv: enabled, distance, frequency, interval, accuracy
  }
#endif

  if (c == KEY_UP || c == KEY_PREV) {
    if (_selected_setting > 0) {
      _selected_setting--;
    } else {
      _selected_setting = num_settings - 1; // Wrap to bottom
    }
    return true;
  }

  if (c == KEY_DOWN || c == KEY_NEXT) {
    _selected_setting = (_selected_setting + 1) % num_settings;
    return true;
  }

  if (c == KEY_CANCEL) {
    // Back to home
    _task->gotoHomeScreen();
    return true;
  }

  if (c == KEY_ENTER || c == KEY_SELECT) {
    // Toggle the selected setting
    int curr_setting = 0;

    // BLE (setting 0)
    if (curr_setting == _selected_setting) {
      if (_task->isSerialEnabled()) {
        _task->disableSerial();
        _task->notify(UIEventType::ack);
        _task->showAlert("BLE: OFF", 800);
      } else {
        _task->enableSerial();
        _task->notify(UIEventType::ack);
        _task->showAlert("BLE: ON", 800);
      }
      return true;
    }
    curr_setting++;

    // Buzzer (setting 1)
    if (curr_setting == _selected_setting) {
      _task->toggleBuzzer();
      return true;
    }
    curr_setting++;

    // Key Press Buzzer (setting 2)
    if (curr_setting == _selected_setting) {
#ifdef PIN_BUZZER
      _node_prefs->buzzer_key_press = !_node_prefs->buzzer_key_press;
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
      _task->showAlert(_node_prefs->buzzer_key_press ? "Key Buzz: ON" : "Key Buzz: OFF", 800);
#endif
      return true;
    }
    curr_setting++;

#if ENV_INCLUDE_GPS == 1
    if (_sensors->getLocationProvider() != NULL) {
      // GPS Toggle (setting 3)
      if (curr_setting == _selected_setting) {
        _task->toggleGPS();
        return true;
      }
      curr_setting++;

      // GPS LocAdv settings start here (setting 4+)
      // GPS LocAdv Enabled (setting 4)
      if (curr_setting == _selected_setting) {
        _node_prefs->gps_loc_advert_enabled = !_node_prefs->gps_loc_advert_enabled;
        the_mesh.savePrefs();
        _task->notify(UIEventType::ack);
        _task->showAlert(_node_prefs->gps_loc_advert_enabled ? "LocAdv: ON" : "LocAdv: OFF", 800);
        return true;
      }
      curr_setting++;

      // GPS LocAdv Distance (setting 5)
      if (curr_setting == _selected_setting) {
        // Cycle through common values: 10, 50, 100, 200, 500
        int distances[] = {10, 50, 100, 200, 500};
        int num_distances = sizeof(distances) / sizeof(distances[0]);
        int current_idx = 0;
        for (int i = 0; i < num_distances; i++) {
          if (_node_prefs->gps_loc_distance_threshold == distances[i]) {
            current_idx = i;
            break;
          }
        }
        current_idx = (current_idx + 1) % num_distances;
        _node_prefs->gps_loc_distance_threshold = distances[current_idx];
        the_mesh.savePrefs();
        _task->notify(UIEventType::ack);
        char alert[40];
        sprintf(alert, "Distance: %dm", _node_prefs->gps_loc_distance_threshold);
        _task->showAlert(alert, 800);
        return true;
      }
      curr_setting++;

      // GPS LocAdv Frequency (setting 6)
      if (curr_setting == _selected_setting) {
        // Cycle through values: 30s, 60s, 120s, 300s (stored as value * 10)
        int frequencies[] = {3, 6, 12, 30}; // *10 = 30s, 60s, 120s, 300s
        int num_frequencies = sizeof(frequencies) / sizeof(frequencies[0]);
        int current_idx = 0;
        for (int i = 0; i < num_frequencies; i++) {
          if (_node_prefs->gps_loc_frequency == frequencies[i]) {
            current_idx = i;
            break;
          }
        }
        current_idx = (current_idx + 1) % num_frequencies;
        _node_prefs->gps_loc_frequency = frequencies[current_idx];
        the_mesh.savePrefs();
        _task->notify(UIEventType::ack);
        char alert[40];
        sprintf(alert, "Frequency: %ds", _node_prefs->gps_loc_frequency * 10);
        _task->showAlert(alert, 800);
        return true;
      }
      curr_setting++;

      // GPS LocAdv Interval (setting 7)
      if (curr_setting == _selected_setting) {
        _node_prefs->gps_loc_guaranteed_interval = (_node_prefs->gps_loc_guaranteed_interval + 1) % 3;
        the_mesh.savePrefs();
        _task->notify(UIEventType::ack);
        const char* interval_str;
        switch (_node_prefs->gps_loc_guaranteed_interval) {
        case 0:
          interval_str = "1m";
          break;
        case 1:
          interval_str = "5m";
          break;
        case 2:
          interval_str = "15m";
          break;
        default:
          interval_str = "?";
          break;
        }
        char alert[40];
        sprintf(alert, "Interval: %s", interval_str);
        _task->showAlert(alert, 800);
        return true;
      }
      curr_setting++;

      // GPS LocAdv Accuracy (setting 8)
      if (curr_setting == _selected_setting) {
        // Cycle through common values: 5, 10, 20, 50, 100
        int accuracies[] = {5, 10, 20, 50, 100};
        int num_accuracies = sizeof(accuracies) / sizeof(accuracies[0]);
        int current_idx = 0;
        for (int i = 0; i < num_accuracies; i++) {
          if (_node_prefs->gps_loc_accuracy_threshold == accuracies[i]) {
            current_idx = i;
            break;
          }
        }
        current_idx = (current_idx + 1) % num_accuracies;
        _node_prefs->gps_loc_accuracy_threshold = accuracies[current_idx];
        the_mesh.savePrefs();
        _task->notify(UIEventType::ack);
        char alert[40];
        sprintf(alert, "Accuracy: %dm", _node_prefs->gps_loc_accuracy_threshold);
        _task->showAlert(alert, 800);
        return true;
      }
      curr_setting++;
    }
#endif

    // Telemetry (next setting after GPS settings or setting 4)
    if (curr_setting == _selected_setting) {
      // Cycle through DENY -> FLAGS -> ALL -> DENY
      if (_node_prefs->telemetry_mode_base == TELEM_MODE_DENY) {
        _node_prefs->telemetry_mode_base = TELEM_MODE_ALLOW_FLAGS;
      } else if (_node_prefs->telemetry_mode_base == TELEM_MODE_ALLOW_FLAGS) {
        _node_prefs->telemetry_mode_base = TELEM_MODE_ALLOW_ALL;
      } else {
        _node_prefs->telemetry_mode_base = TELEM_MODE_DENY;
      }
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
      const char* mode_str;
      if (_node_prefs->telemetry_mode_base == TELEM_MODE_ALLOW_ALL) {
        mode_str = "ALL";
      } else if (_node_prefs->telemetry_mode_base == TELEM_MODE_ALLOW_FLAGS) {
        mode_str = "FLAGS";
      } else {
        mode_str = "DENY";
      }
      char alert[40];
      sprintf(alert, "Telemetry: %s", mode_str);
      _task->showAlert(alert, 800);
      return true;
    }
    curr_setting++;

    // Advertise Location
    if (curr_setting == _selected_setting) {
      if (_node_prefs->advert_loc_policy == ADVERT_LOC_SHARE) {
        _node_prefs->advert_loc_policy = ADVERT_LOC_NONE;
      } else {
        _node_prefs->advert_loc_policy = ADVERT_LOC_SHARE;
      }
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
      _task->showAlert(_node_prefs->advert_loc_policy == ADVERT_LOC_SHARE ? "Advert Loc: ON" : "Advert Loc: OFF",
                       800);
      return true;
    }
    curr_setting++;

    // Clear Files
    if (curr_setting == _selected_setting) {
      // Save current settings first
      the_mesh.savePrefs();
      _task->notify(UIEventType::ack);
      _task->showAlert("Clearing files...", 1500);

      // Clear all files except settings
      bool success = the_mesh.clearAllFilesExceptSettings();

      if (success) {
        // Reload settings to ensure they're in memory
        _task->showAlert("Files cleared! Rebooting...", 2000);
        delay(2000);
        // Reboot the device
        _task->shutdown(true);  // true = restart
      } else {
        _task->showAlert("Clear failed!", 1500);
      }
      return true;
    }
    curr_setting++;
  }

  return false;
}

void SettingsScreen::poll() {
  // Nothing to poll for settings screen
}
