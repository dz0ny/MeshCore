#include "HomeScreen.h"
#include "UITask.h"
#include "UIHelpers.h"
#include "helpers/ui/UIStrings.h"
#include "icons.h"
#include "../MyMesh.h"
#include <helpers/sensors/LPPDataHelpers.h>
#include <helpers/SensorManager.h>
#include <RTClib.h>

// External references
extern MyMesh the_mesh;

#ifndef UI_RECENT_LIST_SIZE
  #define UI_RECENT_LIST_SIZE 4
#endif

#ifndef AUTO_OFF_MILLIS
  #define AUTO_OFF_MILLIS     15000   // 15 seconds
#endif

#if UI_HAS_JOYSTICK
  #define PRESS_LABEL "press Enter"
#else
  #define PRESS_LABEL "long press"
#endif

HomeScreen::HomeScreen(UITask* task, mesh::RTCClock* rtc, SensorManager* sensors, NodePrefs* node_prefs)
   : _task(task), _rtc(rtc), _sensors(sensors), _node_prefs(node_prefs), _page(0),
     sensors_lpp(200),
     cached_battery_percentage(0),
     cached_battery_millivolts(0),
     next_battery_refresh(0),
     cached_display_width(0),
     cached_display_height(0),
     cached_center_x(0),
     cached_pubkey_valid(false),
     cached_line_height(0),
     cached_line_spacing(0),
     sensors_nb(0),
     sensors_scroll(false),
     sensors_scroll_offset(0),
     next_sensors_refresh(0) {
  cached_filtered_name[0] = '\0';
  cached_node_name_source[0] = '\0';
  cached_pubkey_hex[0] = '\0';
}

HomeScreen::~HomeScreen() {
  // No dynamic allocations to clean up
}

void HomeScreen::setPage(HomePage page) {
  _page = page;
}

void HomeScreen::poll() {
  // Nothing to poll for home screen
}

void HomeScreen::renderBatteryIndicator(DisplayDriver& display, uint16_t batteryMilliVolts) {
  // Update battery percentage only every 5 minutes or if voltage changed significantly
  if (millis() > next_battery_refresh || abs((int)batteryMilliVolts - (int)cached_battery_millivolts) > 100) {
    const int minMilliVolts = 3000; // Minimum voltage (e.g., 3.0V)
    const int maxMilliVolts = 4200; // Maximum voltage (e.g., 4.2V)
    cached_battery_percentage = ((batteryMilliVolts - minMilliVolts) * 100) / (maxMilliVolts - minMilliVolts);
    if (cached_battery_percentage < 0) cached_battery_percentage = 0; // Clamp to 0%
    if (cached_battery_percentage > 100) cached_battery_percentage = 100; // Clamp to 100%
    cached_battery_millivolts = batteryMilliVolts;
    next_battery_refresh = millis() + 300000; // Refresh every 5 minutes (300 seconds)
  }

  // Battery icon - positioned at top
  const int batteryWidth = 32;
  const int batteryHeight = 14;
  const int batteryX = display.width() - batteryWidth - 5;
  const int batteryY = 0;

  display.setColor(DisplayDriver::GREEN);
  display.drawRect(batteryX, batteryY, batteryWidth, batteryHeight);
  display.fillRect(batteryX + batteryWidth, batteryY + (batteryHeight / 4), 3, batteryHeight / 2);
  int fillWidth = (cached_battery_percentage * (batteryWidth - 4)) / 100;
  display.fillRect(batteryX + 2, batteryY + 2, fillWidth, batteryHeight - 4);
}

void HomeScreen::refresh_sensors() {
  if (millis() > next_sensors_refresh) {
    sensors_lpp.reset();
    sensors_nb = 0;
    sensors_lpp.addVoltage(TELEM_CHANNEL_SELF, (float)_task->getBattMilliVolts() / 1000.0f);
    _sensors->querySensors(0xFF, sensors_lpp);
    LPPReader reader (sensors_lpp.getBuffer(), sensors_lpp.getSize());
    uint8_t channel, type;
    while(reader.readHeader(channel, type)) {
      reader.skipData(type);
      sensors_nb ++;
    }
    sensors_scroll = sensors_nb > RECENT_BUFFER_SIZE;
#if AUTO_OFF_MILLIS > 0
    next_sensors_refresh = millis() + 5000; // refresh sensor values every 5 sec
#else
    next_sensors_refresh = millis() + 60000; // refresh sensor values every 1 min
#endif
  }
}

int HomeScreen::render(DisplayDriver& display) {
  // Cache display dimensions on first render
  if (cached_display_width == 0) {
    cached_display_width = display.width();
    cached_display_height = display.height();
    cached_center_x = cached_display_width / 2;
  }

  char tmp[80];

  // Screen name (centered at the very top) - use small font (size 0)
  // Skip rendering empty screen name to save cycles
  // const char* screen_name = "";
  // display.setTextSize(0);
  // display.setColor(DisplayDriver::YELLOW);
  // display.drawTextCentered(cached_center_x, 0, screen_name);

  // node name - back to normal size (with caching)
  display.setTextSize(1);
  display.setColor(DisplayDriver::GREEN);
  // Only translate if node name changed
  if (strcmp(_node_prefs->node_name, cached_node_name_source) != 0) {
    strncpy(cached_node_name_source, _node_prefs->node_name, sizeof(cached_node_name_source) - 1);
    cached_node_name_source[sizeof(cached_node_name_source) - 1] = '\0';
    display.translateUTF8ToBlocks(cached_filtered_name, _node_prefs->node_name, sizeof(cached_filtered_name));
  }
  display.setCursor(0, 0);
  display.print(cached_filtered_name);

  // battery voltage and time
  renderBatteryIndicator(display, _task->getBattMilliVolts());

  // Time display (hh:mm) - left of battery
  uint32_t now = _rtc->getCurrentTime();
  if (now > 0) {
    DateTime dt = DateTime(now);
    char timeBuf[6];
    sprintf(timeBuf, "%02d:%02d", dt.hour(), dt.minute());
    display.setColor(DisplayDriver::GREEN);
    display.setTextSize(1);
    const int batteryWidth = 32;
    const int batteryX = display.width() - batteryWidth - 5;
    display.drawTextRightAlign(batteryX - 5, 0, timeBuf);
  }

  // curr page indicator
  static const int indicator_y = 14;
  int x = cached_center_x - 5 * (HomePage::Count-1);
  for (uint8_t i = 0; i < HomePage::Count; i++, x += 10) {
    if (i == _page) {
      display.fillRect(x-1, indicator_y-1, 3, 3);
    } else {
      display.fillRect(x, indicator_y, 1, 1);
    }
  }

  // Draw dotted line separator between header and content
  static const int separator_y = 18;
  display.setColor(DisplayDriver::LIGHT);
  drawDottedHLine(display, 0, separator_y, cached_display_width, 3);

  // Pre-calculate commonly used values for page content
  int available_height = cached_display_height - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN;
  int centerY = SCREEN_TOP_MARGIN + (available_height / 2);

  if (_page == HomePage::FIRST) {

    if (_task->hasConnection()) {
      display.setColor(DisplayDriver::GREEN);
      display.setTextSize(1);
      display.drawTextCentered(cached_center_x, centerY, TR(STR_CONNECTED));
    } else if (the_mesh.getBLEPin() != 0) { // BT pin
      display.setColor(DisplayDriver::RED);
      display.setTextSize(2);
      char fmt[32];
      strcpy(fmt, TR(STR_PIN));
      sprintf(tmp, fmt, the_mesh.getBLEPin());
      display.drawTextCentered(cached_center_x, centerY - 8, tmp);
      display.setTextSize(1);
      display.setColor(DisplayDriver::LIGHT);
      display.drawTextCentered(cached_center_x, centerY + 12, TR(STR_ENTER_WHEN_PAIRING));
    }
  } else if (_page == HomePage::CONTACTS) {
    // CONTACTS page - show preview with count
    the_mesh.getRecentlyHeard(recent, RECENT_BUFFER_SIZE);

    // Count non-empty contacts
    int contact_count = 0;
    for (int i = 0; i < RECENT_BUFFER_SIZE; i++) {
      if (recent[i].name[0] != 0) contact_count++;
    }

    display.setColor(DisplayDriver::GREEN);
    display.setTextSize(2);
    static char contact_buf[32];
    if (contact_count > 0) {
      snprintf(contact_buf, sizeof(contact_buf), TR(STR_NEARBY), contact_count);
      display.drawTextCentered(cached_center_x, centerY - 10, contact_buf);
    } else {
      display.drawTextCentered(cached_center_x, centerY - 10, TR(STR_NO_CONTACTS));
    }

    display.setTextSize(1);
    display.setColor(DisplayDriver::LIGHT);
    display.drawTextCentered(cached_center_x, centerY + 10, TR(STR_PRESS_TO_VIEW));
  } else if (_page == HomePage::SETTINGS) {
    // Settings page - show big "Settings" text similar to Messages
    display.setColor(DisplayDriver::GREEN);
    display.setTextSize(2);
    display.drawTextCentered(cached_center_x, centerY - 10, TR(STR_SETTINGS));

    display.setTextSize(1);
    display.setColor(DisplayDriver::LIGHT);
    display.drawTextCentered(cached_center_x, centerY + 10, TR(STR_PRESS_TO_VIEW));
  } else if (_page == HomePage::MESSAGES) {
    // Messages page - show message count and preview
    display.setColor(DisplayDriver::GREEN);
    display.setTextSize(1);

    uint8_t msg_count = _task->getMessagesScreen()->getUnreadCount();
    uint8_t total_count = the_mesh.getMessageStore()->getCount();

    if (total_count == 0) {
      // No messages
      display.setTextSize(2);
      display.drawTextCentered(cached_center_x, centerY - 10, TR(STR_NO_MESSAGES));
      display.setTextSize(1);
      display.drawTextCentered(cached_center_x, centerY + 10, TR(STR_PRESS_TO_VIEW));
    } else {
      // Show message count - use static buffer
      display.setTextSize(2);
      static char buf[32];
      if (msg_count > 0) {
        snprintf(buf, sizeof(buf), TR(STR_UNREAD), msg_count);
        display.drawTextCentered(cached_center_x, centerY - 10, buf);
      } else {
        snprintf(buf, sizeof(buf), TR(STR_TOTAL_COUNT), total_count);
        display.drawTextCentered(cached_center_x, centerY - 10, buf);
      }

      display.setTextSize(1);
      display.setColor(DisplayDriver::LIGHT);
      display.drawTextCentered(cached_center_x, centerY + 10, TR(STR_PRESS_TO_VIEW));
    }
  } else if (_page == HomePage::ADVERT) {
    // ADVERT page - show public key hash, location, and status
    display.setColor(DisplayDriver::LIGHT);

    // Cache line height calculations (done once per display)
    if (cached_line_height == 0) {
      cached_line_height = display.getTextHeight("A");
      cached_line_spacing = cached_line_height + 2;
    }

    // Calculate vertical centering for content
    // Lines: pubkey, lat, lon, and if GPS enabled: fix+sats (same line)
    LocationProvider* nmea = _sensors->getLocationProvider();
    int num_lines = 3; // pubkey, lat, lon
    if (nmea != NULL && nmea->isEnabled()) {
      num_lines += 1; // fix and sats on same line
    }
    int total_height = (num_lines * cached_line_height) + ((num_lines - 1) * 2);
    int y = SCREEN_TOP_MARGIN + ((available_height - total_height) / 2) + (cached_line_height / 2);

    char buf[50];

    // Public key hash (first 8 hex chars) - cache permanently
    if (!cached_pubkey_valid) {
      const uint8_t* pubkey = the_mesh.getMyPubKey();
      mesh::Utils::toHex(cached_pubkey_hex, pubkey, 4);
      cached_pubkey_valid = true;
    }
    display.setColor(DisplayDriver::GREEN);
    display.drawTextLeftAlign(0, y, TR(STR_ID));
    display.setColor(DisplayDriver::LIGHT);
    display.drawTextRightAlign(cached_display_width - 1, y, cached_pubkey_hex);

    y += cached_line_spacing;

    // Location information
    bool has_gps_fix = nmea != NULL && nmea->isValid();

    // Latitude
    display.setColor(DisplayDriver::GREEN);
    display.drawTextLeftAlign(0, y, TR(STR_LAT));
    display.setColor(DisplayDriver::LIGHT);
    if (has_gps_fix) {
      sprintf(buf, "%.5f", nmea->getLatitude() / 1000000.);
      display.drawTextRightAlign(cached_display_width - 1, y, buf);
    } else if (_sensors->node_lat != 0) {
      // Show configured latitude
      sprintf(buf, "%.5f", _sensors->node_lat);
      display.drawTextRightAlign(cached_display_width - 1, y, buf);
    } else {
      display.drawTextRightAlign(cached_display_width - 1, y, "-");
    }

    y += cached_line_spacing;

    // Longitude
    display.setColor(DisplayDriver::GREEN);
    display.drawTextLeftAlign(0, y, TR(STR_LON));
    display.setColor(DisplayDriver::LIGHT);
    if (has_gps_fix) {
      sprintf(buf, "%.5f", nmea->getLongitude() / 1000000.);
      display.drawTextRightAlign(cached_display_width - 1, y, buf);
    } else if (_sensors->node_lon != 0) {
      // Show configured longitude
      sprintf(buf, "%.5f", _sensors->node_lon);
      display.drawTextRightAlign(cached_display_width - 1, y, buf);
    } else {
      display.drawTextRightAlign(cached_display_width - 1, y, "-");
    }

    y += cached_line_spacing;

    // GPS accuracy and satellite count on same line (if GPS enabled)
    if (nmea != NULL && nmea->isEnabled()) {
      // Left side: GPS Accuracy in meters
      display.setColor(DisplayDriver::GREEN);
      display.drawTextLeftAlign(0, y, TR(STR_FIX));
      display.setColor(DisplayDriver::LIGHT);
      char acc_buf[10];
      if (has_gps_fix) {
        float accuracy = nmea->getAccuracy();
        sprintf(acc_buf, "%.0fm", accuracy);
      } else {
        strcpy(acc_buf, "--m");
      }
      // Calculate position after accuracy label
      char fix_label_space[10];
      strcpy(fix_label_space, TR(STR_FIX));
      strcat(fix_label_space, " ");
      int acc_x = display.getTextWidth(fix_label_space) + 2;
      display.drawTextLeftAlign(acc_x, y, acc_buf);

      // Right side: Number of satellites
      long sat_count = nmea->satellitesCount();
      char sat_buf[20];
      sprintf(sat_buf, "%s %ld", TR(STR_SATS), sat_count);
      display.setColor(DisplayDriver::GREEN);
      display.drawTextRightAlign(cached_display_width - 1, y, sat_buf);
    }

    // Show instruction at bottom
    y += cached_line_spacing + 4;
    display.setColor(DisplayDriver::LIGHT);
    display.drawTextCentered(cached_center_x, y, TR(STR_SEND_LOCATION));
  } else if (_page == HomePage::REPORTS) {
    // Reports page - show preview
    display.setColor(DisplayDriver::GREEN);
    display.setTextSize(2);
    display.drawTextCentered(cached_center_x, centerY - 10, TR(STR_REPORTS));

    display.setTextSize(1);
    display.setColor(DisplayDriver::LIGHT);
    display.drawTextCentered(cached_center_x, centerY + 10, TR(STR_PRESS_TO_VIEW));
  }
  return 5000;   // next render after 5000 ms
}

bool HomeScreen::handleInput(char c) {
  // Back button always goes to ADVERT page
  if (c == KEY_CANCEL) {
    _page = HomePage::ADVERT;
    return true;
  }

  // Handle FIRST page - if connected, any key jumps to ADVERT page
  if (_page == HomePage::FIRST && _task->hasConnection()) {
    // Any key press when connected jumps to advert screen
    _page = HomePage::ADVERT;
    return true;
  }

  // Handle SETTINGS page - press to enter settings screen
  if (_page == HomePage::SETTINGS) {
    if (c == KEY_ENTER || c == KEY_SELECT) {
      _task->gotoSettingsScreen();
      return true;
    }
  }

  // Handle page navigation
  if (c == KEY_LEFT || c == KEY_PREV) {
    _page = (_page + HomePage::Count - 1) % HomePage::Count;
    return true;
  }
  if (c == KEY_NEXT || c == KEY_RIGHT) {
    _page = (_page + 1) % HomePage::Count;
    return true;
  }
  if ((c == KEY_ENTER || c == KEY_SELECT) && _page == HomePage::MESSAGES) {
    // Switch to full messages screen
    _task->gotoMessagesScreen();
    return true;
  }
  if ((c == KEY_ENTER || c == KEY_SELECT) && _page == HomePage::CONTACTS) {
    // Switch to full contacts screen
    _task->gotoContactsScreen();
    return true;
  }
  if (c == KEY_ENTER && _page == HomePage::ADVERT) {
    _task->notify(UIEventType::ack);
    if (the_mesh.advert()) {
      _task->showAlert(TR(STR_ADVERT_SENT), 1000);
    } else {
      _task->showAlert(TR(STR_ADVERT_FAILED), 1000);
    }
    return true;
  }
  if ((c == KEY_ENTER || c == KEY_SELECT) && _page == HomePage::REPORTS) {
    // Switch to full reports screen
    _task->gotoReportsScreen();
    return true;
  }
  return false;
}
