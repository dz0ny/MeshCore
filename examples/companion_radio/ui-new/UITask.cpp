#include "UITask.h"
#include <helpers/TxtDataHelpers.h>
#include "../MyMesh.h"
#include "target.h"
#include <math.h>

extern MyMesh the_mesh;

#ifdef MESHTASTIC_INCLUDE_INKHUD
#include <helpers/ui/BatteryIconApplet.h>
#endif

#ifndef AUTO_OFF_MILLIS
  #define AUTO_OFF_MILLIS     15000   // 15 seconds
#endif
#define BOOT_SCREEN_MILLIS   3000   // 3 seconds

#ifdef PIN_STATUS_LED
#define LED_ON_MILLIS     20
#define LED_ON_MSG_MILLIS 200
#define LED_CYCLE_MILLIS  4000
#endif

#define LONG_PRESS_MILLIS   1200

#ifndef UI_RECENT_LIST_SIZE
  #define UI_RECENT_LIST_SIZE 4
#endif

#if UI_HAS_JOYSTICK
  #define PRESS_LABEL "press Enter"
#else
  #define PRESS_LABEL "long press"
#endif

#include "icons.h"

class SplashScreen : public UIScreen {
  UITask* _task;
  unsigned long dismiss_after;
  char _version_info[12];

public:
  SplashScreen(UITask* task) : _task(task) {
    // strip off dash and commit hash by changing dash to null terminator
    // e.g: v1.2.3-abcdef -> v1.2.3
    const char *ver = FIRMWARE_VERSION;
    const char *dash = strchr(ver, '-');

    int len = dash ? dash - ver : strlen(ver);
    if (len >= sizeof(_version_info)) len = sizeof(_version_info) - 1;
    memcpy(_version_info, ver, len);
    _version_info[len] = 0;

    dismiss_after = millis() + BOOT_SCREEN_MILLIS;
  }

  int render(DisplayDriver& display) override {
    // Calculate vertical center with margins
    const int SCREEN_TOP_MARGIN = 5;
    const int SCREEN_BOTTOM_MARGIN = 5;
    int available_height = display.height() - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN;
    int centerY = SCREEN_TOP_MARGIN + (available_height / 2);

    // meshcore logo
    display.setColor(DisplayDriver::BLUE);
    int logoWidth = 128;
    int logoHeight = 13;
    display.drawXbm((display.width() - logoWidth) / 2, centerY - 25, meshcore_logo, logoWidth, logoHeight);

    // version info
    display.setColor(DisplayDriver::LIGHT);
    display.setTextSize(2);
    display.drawTextCentered(display.width()/2, centerY - 5, _version_info);

    display.setTextSize(1);
    display.drawTextCentered(display.width()/2, centerY + 15, FIRMWARE_BUILD_DATE);

    return 1000;
  }

  void poll() override {
    if (millis() >= dismiss_after) {
      _task->gotoHomeScreen();
    }
  }
};

class HomeScreen : public UIScreen {
public:
  enum HomePage {
    FIRST,
    SETTINGS,
    MESSAGES,
    NEARBY,
    ADVERT,
#if UI_SENSORS_PAGE == 1
    SENSORS,
#endif
    SHUTDOWN,
    Count    // keep as last
  };

private:
  UITask* _task;
  mesh::RTCClock* _rtc;
  SensorManager* _sensors;
  NodePrefs* _node_prefs;
  uint8_t _page;
  bool _shutdown_init;
  AdvertPath recent[UI_RECENT_LIST_SIZE];

  // Screen layout constants
  static constexpr int SCREEN_TOP_MARGIN = 20;  // Header height (18px) + separator + spacing
  static constexpr int SCREEN_BOTTOM_MARGIN = 18;


  void renderBatteryIndicator(DisplayDriver& display, uint16_t batteryMilliVolts) {
#ifdef MESHTASTIC_INCLUDE_INKHUD
    // Use the new InkHUD BatteryIconApplet for ink/e-paper displays
    static InkHUD::BatteryIconApplet* batteryIcon = nullptr;

    if (batteryIcon == nullptr) {
      // Create battery icon positioned at top
      batteryIcon = new InkHUD::BatteryIconApplet(
        &display,
        display.width() - 40 - 5,  // x: 5 pixels from right edge
        0,                          // y: at top with screen name
        40,                         // width
        20                          // height
      );
    }

    // Update and render
    batteryIcon->updateBatteryMilliVolts(batteryMilliVolts);
    batteryIcon->render();
#else
    // Original battery rendering for non-InkHUD displays
    // Convert millivolts to percentage
    const int minMilliVolts = 3000; // Minimum voltage (e.g., 3.0V)
    const int maxMilliVolts = 4200; // Maximum voltage (e.g., 4.2V)
    int batteryPercentage = ((batteryMilliVolts - minMilliVolts) * 100) / (maxMilliVolts - minMilliVolts);
    if (batteryPercentage < 0) batteryPercentage = 0; // Clamp to 0%
    if (batteryPercentage > 100) batteryPercentage = 100; // Clamp to 100%

    // battery icon - positioned at top
    int iconWidth = 32;
    int iconHeight = 14;
    int iconX = display.width() - iconWidth - 5; // Position the icon near the top-right corner
    int iconY = 0;  // At top with screen name
    display.setColor(DisplayDriver::GREEN);

    // battery outline
    display.drawRect(iconX, iconY, iconWidth, iconHeight);

    // battery "cap"
    display.fillRect(iconX + iconWidth, iconY + (iconHeight / 4), 3, iconHeight / 2);

    // fill the battery based on the percentage
    int fillWidth = (batteryPercentage * (iconWidth - 4)) / 100;
    display.fillRect(iconX + 2, iconY + 2, fillWidth, iconHeight - 4);
#endif
  }

  CayenneLPP sensors_lpp;
  int sensors_nb = 0;
  bool sensors_scroll = false;
  int sensors_scroll_offset = 0;
  int next_sensors_refresh = 0;

  void refresh_sensors() {
    if (millis() > next_sensors_refresh) {
      sensors_lpp.reset();
      sensors_nb = 0;
      sensors_lpp.addVoltage(TELEM_CHANNEL_SELF, (float)board.getBattMilliVolts() / 1000.0f);
      sensors.querySensors(0xFF, sensors_lpp);
      LPPReader reader (sensors_lpp.getBuffer(), sensors_lpp.getSize());
      uint8_t channel, type;
      while(reader.readHeader(channel, type)) {
        reader.skipData(type);
        sensors_nb ++;
      }
      sensors_scroll = sensors_nb > UI_RECENT_LIST_SIZE;
#if AUTO_OFF_MILLIS > 0
      next_sensors_refresh = millis() + 5000; // refresh sensor values every 5 sec
#else
      next_sensors_refresh = millis() + 60000; // refresh sensor values every 1 min
#endif
    }
  }

public:
  HomeScreen(UITask* task, mesh::RTCClock* rtc, SensorManager* sensors, NodePrefs* node_prefs)
     : _task(task), _rtc(rtc), _sensors(sensors), _node_prefs(node_prefs), _page(0),
       _shutdown_init(false), sensors_lpp(200) {  }

  void setPage(HomePage page) { _page = page; }

  void poll() override {
    if (_shutdown_init && !_task->isButtonPressed()) {  // must wait for USR button to be released
      _task->shutdown();
    }
  }

  int render(DisplayDriver& display) override {
    char tmp[80];

    // Screen name (centered at the very top) - use small font (size 0)
    const char* screen_name = "";
  

    // Draw screen name centered at the very top with smallest font
    display.setTextSize(0);  // Use smallest font (7pt for e-ink, 5px for OLED)
    display.setColor(DisplayDriver::YELLOW);
    display.drawTextCentered(display.width() / 2, 0, screen_name);

    // node name - back to normal size
    display.setTextSize(1);
    display.setColor(DisplayDriver::GREEN);
    char filtered_name[sizeof(_node_prefs->node_name)];
    display.translateUTF8ToBlocks(filtered_name, _node_prefs->node_name, sizeof(filtered_name));
    display.setCursor(0, 0);
    display.print(filtered_name);

    // battery voltage
    renderBatteryIndicator(display, _task->getBattMilliVolts());

    // curr page indicator
    int y = 14;
    int x = display.width() / 2 - 5 * (HomePage::Count-1);
    for (uint8_t i = 0; i < HomePage::Count; i++, x += 10) {
      if (i == _page) {
        display.fillRect(x-1, y-1, 3, 3);
      } else {
        display.fillRect(x, y, 1, 1);
      }
    }

    // Draw dotted line separator between header and content
    int separator_y = 18;
    display.setColor(DisplayDriver::LIGHT);
    for (int dx = 0; dx < display.width(); dx += 3) {
      display.fillRect(dx, separator_y, 1, 1);
    }

    if (_page == HomePage::FIRST) {
      int available_height = display.height() - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN;
      int centerY = SCREEN_TOP_MARGIN + (available_height / 2);

      if (_task->hasConnection()) {
        display.setColor(DisplayDriver::GREEN);
        display.setTextSize(1);
        display.drawTextCentered(display.width() / 2, centerY, "< Connected >");
      } else if (the_mesh.getBLEPin() != 0) { // BT pin
        display.setColor(DisplayDriver::RED);
        display.setTextSize(2);
        sprintf(tmp, "Pin:%d", the_mesh.getBLEPin());
        display.drawTextCentered(display.width() / 2, centerY, tmp);
      }
    } else if (_page == HomePage::SETTINGS) {
      // Settings page - show big "Settings" text similar to Messages
      display.setColor(DisplayDriver::GREEN);
      display.setTextSize(2);

      int available_height = display.height() - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN;
      int centerY = SCREEN_TOP_MARGIN + (available_height / 2);

      // Show "Settings" text
      display.setTextSize(2);
      display.drawTextCentered(display.width() / 2, centerY - 10, "Settings");

      display.setTextSize(1);
      display.setColor(DisplayDriver::LIGHT);
      display.drawTextCentered(display.width() / 2, centerY + 10, "Press to view");
    } else if (_page == HomePage::MESSAGES) {
      // Messages page - show message count and preview
      display.setColor(DisplayDriver::GREEN);
      display.setTextSize(1);

      uint8_t msg_count = _task->getMessagesScreen()->getUnreadCount();
      uint8_t total_count = the_mesh.getMessageStore()->getCount();

      int available_height = display.height() - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN;
      int centerY = SCREEN_TOP_MARGIN + (available_height / 2);

      if (total_count == 0) {
        // No messages
        display.setTextSize(2);
        display.drawTextCentered(display.width() / 2, centerY - 10, "No Messages");
        display.setTextSize(1);
        display.drawTextCentered(display.width() / 2, centerY + 10, "Press to view");
      } else {
        // Show message count
        display.setTextSize(2);
        char buf[32];
        if (msg_count > 0) {
          snprintf(buf, sizeof(buf), "%d unread", msg_count);
          display.drawTextCentered(display.width() / 2, centerY - 20, buf);
        }
        snprintf(buf, sizeof(buf), "%d total", total_count);
        display.setTextSize(1);
        display.drawTextCentered(display.width() / 2, centerY, buf);

        // Show newest message preview
        const StoredMessage* newest = the_mesh.getMessageStore()->getMessage(total_count - 1);
        if (newest) {
          display.setColor(DisplayDriver::YELLOW);
          display.drawTextCentered(display.width() / 2, centerY + 15, newest->sender_name);

          display.setColor(DisplayDriver::LIGHT);
          char preview[25];
          int preview_len = (strlen(newest->msg) > 20) ? 20 : strlen(newest->msg);
          strncpy(preview, newest->msg, preview_len);
          preview[preview_len] = '\0';
          if (strlen(newest->msg) > 20) {
            strcat(preview, "...");
          }
          display.drawTextCentered(display.width() / 2, centerY + 25, preview);
        }

        display.setTextSize(1);
        display.setColor(DisplayDriver::LIGHT);
        display.drawTextCentered(display.width() / 2, centerY + 40, "Press to view");
      }
    } else if (_page == HomePage::NEARBY) {
      // Nearby page - show preview similar to Messages/Settings
      the_mesh.getRecentlyHeard(recent, UI_RECENT_LIST_SIZE);
      display.setColor(DisplayDriver::GREEN);

      // Count nearby nodes
      int nearby_count = 0;
      for (int i = 0; i < UI_RECENT_LIST_SIZE; i++) {
        if (recent[i].name[0] != 0) nearby_count++;
      }

      int available_height = display.height() - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN;
      int centerY = SCREEN_TOP_MARGIN + (available_height / 2);

      if (nearby_count == 0) {
        // No nearby nodes
        display.setTextSize(2);
        display.drawTextCentered(display.width() / 2, centerY - 10, "No Nodes");
        display.setTextSize(1);
        display.setColor(DisplayDriver::LIGHT);
        display.drawTextCentered(display.width() / 2, centerY + 10, "Press to view");
      } else {
        // Show count
        display.setTextSize(2);
        char count_buf[32];
        snprintf(count_buf, sizeof(count_buf), "%d nearby", nearby_count);
        display.drawTextCentered(display.width() / 2, centerY - 10, count_buf);

        // Show newest node name
        if (recent[0].name[0] != 0) {
          display.setColor(DisplayDriver::YELLOW);
          display.setTextSize(1);
          char filtered_name[sizeof(recent[0].name)];
          display.translateUTF8ToBlocks(filtered_name, recent[0].name, sizeof(filtered_name));
          display.drawTextCentered(display.width() / 2, centerY + 5, filtered_name);
        }

        display.setTextSize(1);
        display.setColor(DisplayDriver::LIGHT);
        display.drawTextCentered(display.width() / 2, centerY + 20, "Press to view");
      }
    } else if (_page == HomePage::ADVERT) {
      display.setColor(DisplayDriver::GREEN);
      int text_height = display.getTextHeight("A");
      // Total content: 32px icon + 4px gap + text height
      int total_height = 32 + 4 + text_height;
      int available_height = display.height() - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN;
      int centerY = SCREEN_TOP_MARGIN + ((available_height - total_height) / 2);
      display.drawXbm((display.width() - 32) / 2, centerY, advert_icon, 32, 32);
      display.drawTextCentered(display.width() / 2, centerY + 32 + 4 + text_height, "advert: " PRESS_LABEL);
#if UI_SENSORS_PAGE == 1
    } else if (_page == HomePage::SENSORS) {
      int line_height = display.getTextHeight("A");
      int line_spacing = line_height + 2;  // font height + small gap
      refresh_sensors();
      // Center the sensor list vertically
      int num_lines = (sensors_scroll ? UI_RECENT_LIST_SIZE : sensors_nb);
      if (num_lines > UI_RECENT_LIST_SIZE) num_lines = UI_RECENT_LIST_SIZE;
      int total_height = num_lines * line_spacing;
      int y = (display.height() - total_height) / 2 + (line_height / 2);
      char buf[30];
      char name[30];
      LPPReader r(sensors_lpp.getBuffer(), sensors_lpp.getSize());

      for (int i = 0; i < sensors_scroll_offset; i++) {
        uint8_t channel, type;
        r.readHeader(channel, type);
        r.skipData(type);
      }

      for (int i = 0; i < (sensors_scroll?UI_RECENT_LIST_SIZE:sensors_nb); i++) {
        uint8_t channel, type;
        if (!r.readHeader(channel, type)) { // reached end, reset
          r.reset();
          r.readHeader(channel, type);
        }

        display.setCursor(0, y);
        float v;
        switch (type) {
          case LPP_GPS: { // GPS
            float lat, lon, alt;
            r.readGPS(lat, lon, alt);

            // Check if GPS data should be displayed based on accuracy
            LocationProvider* gps = _sensors->getLocationProvider();
            bool show_gps = false;

            if (gps != NULL && gps->isValid()) {
              float accuracy = gps->getAccuracy();
              // Only show GPS telemetry if accuracy is better than 20 meters
              show_gps = (accuracy > 0 && accuracy <= 20.0f);
            }

            if (!show_gps) {
              // Skip GPS data display - accuracy too poor or no GPS
              continue;
            }

            // Display latitude
            strcpy(name, "latitude"); sprintf(buf, "%.5f", lat);
            display.setCursor(0, y);
            display.print(name);
            display.setCursor(display.width()-display.getTextWidth(buf)-1, y);
            display.print(buf);
            y = y + line_spacing;
            i++; // Count this as an additional line

            // Check if we have room for more lines
            if (i >= (sensors_scroll?UI_RECENT_LIST_SIZE:sensors_nb)) break;

            // Display longitude
            strcpy(name, "longitude"); sprintf(buf, "%.5f", lon);
            display.setCursor(0, y);
            display.print(name);
            display.setCursor(display.width()-display.getTextWidth(buf)-1, y);
            display.print(buf);
            y = y + line_spacing;
            i++; // Count this as an additional line

            // Check if we have room for more lines and altitude is valid
            if (i >= (sensors_scroll?UI_RECENT_LIST_SIZE:sensors_nb)) break;

            // Display altitude only if valid (non-zero or has meaningful value)
            if (alt != 0.0f) {
              strcpy(name, "altitude"); sprintf(buf, "%.0fm", alt);
            } else {
              // Skip altitude display if it's zero/invalid
              continue;
            }
            break;
          }
          case LPP_VOLTAGE:
            r.readVoltage(v);
            strcpy(name, "voltage"); sprintf(buf, "%6.2f", v);
            break;
          case LPP_CURRENT:
            r.readCurrent(v);
            strcpy(name, "current"); sprintf(buf, "%.3f", v);
            break;
          case LPP_TEMPERATURE:
            r.readTemperature(v);
            strcpy(name, "temperature"); sprintf(buf, "%.2f", v);
            break;
          case LPP_RELATIVE_HUMIDITY:
            r.readRelativeHumidity(v);
            strcpy(name, "humidity"); sprintf(buf, "%.2f", v);
            break;
          case LPP_BAROMETRIC_PRESSURE:
            r.readPressure(v);
            strcpy(name, "pressure"); sprintf(buf, "%.2f", v);
            break;
          case LPP_ALTITUDE:
            r.readAltitude(v);
            strcpy(name, "altitude"); sprintf(buf, "%.0f", v);
            break;
          case LPP_POWER:
            r.readPower(v);
            strcpy(name, "power"); sprintf(buf, "%6.2f", v);
            break;
          default:
            r.skipData(type);
            strcpy(name, "unk"); sprintf(buf, "");
        }
        display.setCursor(0, y);
        display.print(name);
        display.setCursor(
          display.width()-display.getTextWidth(buf)-1, y
        );
        display.print(buf);
        y = y + line_spacing;
      }
      if (sensors_scroll) sensors_scroll_offset = (sensors_scroll_offset+1)%sensors_nb;
      else sensors_scroll_offset = 0;
#endif
    } else if (_page == HomePage::SHUTDOWN) {
      display.setColor(DisplayDriver::GREEN);
      display.setTextSize(1);
      int text_height = display.getTextHeight("A");
      int available_height = display.height() - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN;

      if (_shutdown_init) {
        int centerY = SCREEN_TOP_MARGIN + (available_height / 2);
        display.drawTextCentered(display.width() / 2, centerY, "hibernating...");
      } else {
        // Total content: 32px icon + 4px gap + text height
        int total_height = 32 + 4 + text_height;
        int centerY = SCREEN_TOP_MARGIN + ((available_height - total_height) / 2);
        display.drawXbm((display.width() - 32) / 2, centerY, power_icon, 32, 32);
        display.drawTextCentered(display.width() / 2, centerY + 32 + 4 + text_height, "hibernate:" PRESS_LABEL);
      }
    }
    return 5000;   // next render after 5000 ms
  }

  bool handleInput(char c) override {
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
    if ((c == KEY_ENTER || c == KEY_SELECT) && _page == HomePage::NEARBY) {
      // Switch to full nearby screen
      _task->gotoNearbyScreen();
      return true;
    }
    if (c == KEY_ENTER && _page == HomePage::ADVERT) {
      _task->notify(UIEventType::ack);
      if (the_mesh.advert()) {
        _task->showAlert("Advert sent!", 1000);
      } else {
        _task->showAlert("Advert failed..", 1000);
      }
      return true;
    }
#if UI_SENSORS_PAGE == 1
    if (c == KEY_ENTER && _page == HomePage::SENSORS) {
      _task->toggleGPS();
      next_sensors_refresh=0;
      return true;
    }
#endif
    if (c == KEY_ENTER && _page == HomePage::SHUTDOWN) {
      _shutdown_init = true;  // need to wait for button to be released
      return true;
    }
    return false;
  }
};

class MsgPreviewScreen : public UIScreen {
  UITask* _task;
  mesh::RTCClock* _rtc;

  struct MsgEntry {
    uint32_t timestamp;
    char origin[62];
    char msg[78];
  };
  #define MAX_UNREAD_MSGS   32
  int num_unread;
  MsgEntry unread[MAX_UNREAD_MSGS];

public:
  MsgPreviewScreen(UITask* task, mesh::RTCClock* rtc) : _task(task), _rtc(rtc) { num_unread = 0; }

  void addPreview(uint8_t path_len, const char* from_name, const char* msg) {
    if (num_unread >= MAX_UNREAD_MSGS) return;  // full

    auto p = &unread[num_unread++];
    p->timestamp = _rtc->getCurrentTime();
    if (path_len == 0xFF) {
      sprintf(p->origin, "(D) %s:", from_name);
    } else {
      sprintf(p->origin, "(%d) %s:", (uint32_t) path_len, from_name);
    }
    StrHelper::strncpy(p->msg, msg, sizeof(p->msg));
  }

  int render(DisplayDriver& display) override {
    char tmp[16];
    display.setTextSize(1);
    int line_spacing = display.getTextHeight("A") + 2;  // font height + small gap
    int y = 0;

    display.setCursor(0, y);
    display.setColor(DisplayDriver::GREEN);
    sprintf(tmp, "Unread: %d", num_unread);
    display.print(tmp);

    auto p = &unread[0];

    int secs = _rtc->getCurrentTime() - p->timestamp;
    if (secs < 60) {
      sprintf(tmp, "%ds", secs);
    } else if (secs < 60*60) {
      sprintf(tmp, "%dm", secs / 60);
    } else {
      sprintf(tmp, "%dh", secs / (60*60));
    }
    display.setCursor(display.width() - display.getTextWidth(tmp) - 2, y);
    display.print(tmp);

    y = y + line_spacing;
    display.drawRect(0, y, display.width(), 1);  // horiz line

    y = y + 3;  // small gap after line
    display.setCursor(0, y);
    display.setColor(DisplayDriver::YELLOW);
    char filtered_origin[sizeof(p->origin)];
    display.translateUTF8ToBlocks(filtered_origin, p->origin, sizeof(filtered_origin));
    display.print(filtered_origin);

    y = y + line_spacing;
    display.setCursor(0, y);
    display.setColor(DisplayDriver::LIGHT);
    char filtered_msg[sizeof(p->msg)];
    display.translateUTF8ToBlocks(filtered_msg, p->msg, sizeof(filtered_msg));
    display.printWordWrap(filtered_msg, display.width());

#if AUTO_OFF_MILLIS==0 // probably e-ink
    return 10000; // 10 s
#else
    return 1000;  // next render after 1000 ms
#endif
  }

  bool handleInput(char c) override {
    if (c == KEY_NEXT || c == KEY_RIGHT) {
      num_unread--;
      if (num_unread == 0) {
        _task->gotoHomeScreen();
      } else {
        // delete first/curr item from unread queue
        for (int i = 0; i < num_unread; i++) {
          unread[i] = unread[i + 1];
        }
      }
      return true;
    }
    if (c == KEY_ENTER) {
      num_unread = 0;  // clear unread queue
      _task->gotoHomeScreen();
      return true;
    }
    return false;
  }
};

void UITask::begin(DisplayDriver* display, SensorManager* sensors, NodePrefs* node_prefs) {
  _display = display;
  _sensors = sensors;
  _auto_off = millis() + AUTO_OFF_MILLIS;

#if defined(PIN_USER_BTN)
  user_btn.begin();
#endif
#if UI_HAS_JOYSTICK
  joystick_up.begin();
  joystick_down.begin();
  joystick_left.begin();
  joystick_right.begin();
  back_btn.begin();
#endif
#if defined(PIN_USER_BTN_ANA)
  analog_btn.begin();
#endif

  _node_prefs = node_prefs;
  if (_display != NULL) {
    _display->turnOn();
  }

#ifdef PIN_BUZZER
  buzzer.begin();
#endif

#ifdef PIN_VIBRATION
  vibration.begin();
#endif

  ui_started_at = millis();
  _alert_expiry = 0;

  splash = new SplashScreen(this);
  home = new HomeScreen(this, &rtc_clock, sensors, node_prefs);
  msg_preview = new MsgPreviewScreen(this, &rtc_clock);
  messages = new MessagesScreen(this, the_mesh.getMessageStore());
  nearby = new NearbyScreen(this, &rtc_clock, sensors);
  debug_keys = new DebugKeyScreen(this);
  settings = new SettingsScreen(this, sensors, node_prefs);
  radio_stats = new RadioStatsScreen(this, sensors, node_prefs);
  setCurrScreen(splash);
}

void UITask::showAlert(const char* text, int duration_millis) {
  strcpy(_alert, text);
  _alert_expiry = millis() + duration_millis;
}

void UITask::notify(UIEventType t) {
#if defined(PIN_BUZZER)
switch(t){
  case UIEventType::contactMessage:
    // gemini's pick
    buzzer.play("MsgRcv3:d=4,o=6,b=200:32e,32g,32b,16c7");
    break;
  case UIEventType::channelMessage:
    buzzer.play("kerplop:d=16,o=6,b=120:32g#,32c#");
    break;
  case UIEventType::ack:
    buzzer.play("ack:d=32,o=8,b=120:c");
    break;
  case UIEventType::roomMessage:
  case UIEventType::newContactMessage:
  case UIEventType::none:
  default:
    break;
}
#endif

#ifdef PIN_VIBRATION
  // Trigger vibration for all UI events except none
  if (t != UIEventType::none) {
    vibration.trigger();
  }
#endif
}


void UITask::gotoMessagesHomePage() {
  ((HomeScreen*)home)->setPage(HomeScreen::MESSAGES);
  setCurrScreen(home);
}

void UITask::msgRead(int msgcount) {
  _msgcount = msgcount;
  if (msgcount == 0) {
    gotoHomeScreen();
  }
}

void UITask::newMsg(uint8_t path_len, const char* from_name, const char* text, int msgcount) {
  _msgcount = msgcount;

  // Jump to Messages home page instead of showing msg_preview
  gotoMessagesHomePage();

  if (_display != NULL) {
    if (!_display->isOn()) _display->turnOn();
    _auto_off = millis() + AUTO_OFF_MILLIS;  // extend the auto-off timer
    _next_refresh = 100;  // trigger refresh
  }
}

void UITask::userLedHandler() {
#ifdef PIN_STATUS_LED
  int cur_time = millis();
  if (cur_time > next_led_change) {
    if (led_state == 0) {
      led_state = 1;
      if (_msgcount > 0) {
        last_led_increment = LED_ON_MSG_MILLIS;
      } else {
        last_led_increment = LED_ON_MILLIS;
      }
      next_led_change = cur_time + last_led_increment;
    } else {
      led_state = 0;
      next_led_change = cur_time + LED_CYCLE_MILLIS - last_led_increment;
    }
    digitalWrite(PIN_STATUS_LED, led_state);
  }
#endif
}

void UITask::setCurrScreen(UIScreen* c) {
  curr = c;
  _next_refresh = 100;
}

/*
  hardware-agnostic pre-shutdown activity should be done here
*/
void UITask::shutdown(bool restart){

  #ifdef PIN_BUZZER
  /* note: we have a choice here -
     we can do a blocking buzzer.loop() with non-deterministic consequences
     or we can set a flag and delay the shutdown for a couple of seconds
     while a non-blocking buzzer.loop() plays out in UITask::loop()
  */
  buzzer.shutdown();
  uint32_t buzzer_timer = millis(); // fail-safe shutdown
  while (buzzer.isPlaying() && (millis() - 2500) < buzzer_timer)
    buzzer.loop();

  #endif // PIN_BUZZER

  if (restart) {
    _board->reboot();
  } else {
    _display->turnOff();
    _board->powerOff();
  }
}

bool UITask::isButtonPressed() const {
#ifdef PIN_USER_BTN
  return user_btn.isPressed();
#else
  return false;
#endif
}

void UITask::loop() {
  char c = 0;
#if UI_HAS_JOYSTICK
  int ev = user_btn.check();
  if (ev == BUTTON_EVENT_CLICK) {
    c = checkDisplayOn(KEY_ENTER);
#ifdef PIN_BUZZER
    // Play chirp only if display was already on (c != 0 means event not consumed by wake)
    if (c != 0 && _node_prefs->buzzer_key_press) buzzer.playChirp();
#endif
  } else if (ev == BUTTON_EVENT_LONG_PRESS) {
    c = handleLongPress(KEY_ENTER);  // REVISIT: could be mapped to different key code
  }
  ev = joystick_up.check();
  if (ev == BUTTON_EVENT_CLICK) {
    c = checkDisplayOn(KEY_UP);
#ifdef PIN_BUZZER
    if (c != 0 && _node_prefs->buzzer_key_press) buzzer.playChirp();
#endif
  } else if (ev == BUTTON_EVENT_LONG_PRESS) {
    c = handleLongPress(KEY_UP);
  }
  ev = joystick_down.check();
  if (ev == BUTTON_EVENT_CLICK) {
    c = checkDisplayOn(KEY_DOWN);
#ifdef PIN_BUZZER
    if (c != 0 && _node_prefs->buzzer_key_press) buzzer.playChirp();
#endif
  } else if (ev == BUTTON_EVENT_LONG_PRESS) {
    c = handleLongPress(KEY_DOWN);
  }
  ev = joystick_left.check();
  if (ev == BUTTON_EVENT_CLICK) {
    c = checkDisplayOn(KEY_LEFT);
#ifdef PIN_BUZZER
    if (c != 0 && _node_prefs->buzzer_key_press) buzzer.playChirp();
#endif
  } else if (ev == BUTTON_EVENT_LONG_PRESS) {
    c = handleLongPress(KEY_LEFT);
  }
  ev = joystick_right.check();
  if (ev == BUTTON_EVENT_CLICK) {
    c = checkDisplayOn(KEY_RIGHT);
#ifdef PIN_BUZZER
    if (c != 0 && _node_prefs->buzzer_key_press) buzzer.playChirp();
#endif
  } else if (ev == BUTTON_EVENT_LONG_PRESS) {
    c = handleLongPress(KEY_RIGHT);
  }
  ev = back_btn.check();
  if (ev == BUTTON_EVENT_CLICK) {
    c = checkDisplayOn(KEY_CANCEL);
#ifdef PIN_BUZZER
    if (c != 0 && _node_prefs->buzzer_key_press) buzzer.playChirp();
#endif
  } else if (ev == BUTTON_EVENT_TRIPLE_CLICK) {
    c = handleTripleClick(KEY_SELECT);
  }
#elif defined(PIN_USER_BTN)
  int ev = user_btn.check();
  if (ev == BUTTON_EVENT_CLICK) {
    c = checkDisplayOn(KEY_NEXT);
#ifdef PIN_BUZZER
    // Play chirp only if display was already on (c != 0 means event not consumed by wake)
    if (c != 0 && _node_prefs->buzzer_key_press) buzzer.playChirp();
#endif
  } else if (ev == BUTTON_EVENT_LONG_PRESS) {
    c = handleLongPress(KEY_ENTER);
  } else if (ev == BUTTON_EVENT_DOUBLE_CLICK) {
#ifdef PIN_BUZZER
    if (_node_prefs->buzzer_key_press) buzzer.playComboTune();
#endif
    c = handleDoubleClick(KEY_PREV);
  } else if (ev == BUTTON_EVENT_TRIPLE_CLICK) {
#ifdef PIN_BUZZER
    if (_node_prefs->buzzer_key_press) buzzer.playComboTune();
#endif
    c = handleTripleClick(KEY_SELECT);
  }
#endif
#if defined(PIN_USER_BTN_ANA)
  ev = analog_btn.check();
  if (ev == BUTTON_EVENT_CLICK) {
    c = checkDisplayOn(KEY_NEXT);
#ifdef PIN_BUZZER
    if (c != 0 && _node_prefs->buzzer_key_press) buzzer.playChirp();
#endif
  } else if (ev == BUTTON_EVENT_LONG_PRESS) {
    c = handleLongPress(KEY_ENTER);
  } else if (ev == BUTTON_EVENT_DOUBLE_CLICK) {
#ifdef PIN_BUZZER
    if (_node_prefs->buzzer_key_press) buzzer.playComboTune();
#endif
    c = handleDoubleClick(KEY_PREV);
  } else if (ev == BUTTON_EVENT_TRIPLE_CLICK) {
#ifdef PIN_BUZZER
    if (_node_prefs->buzzer_key_press) buzzer.playComboTune();
#endif
    c = handleTripleClick(KEY_SELECT);
  }
#endif
#if defined(DISP_BACKLIGHT) && defined(BACKLIGHT_BTN)
  if (millis() > next_backlight_btn_check) {
    bool touch_state = digitalRead(PIN_BUTTON2);
    digitalWrite(DISP_BACKLIGHT, !touch_state);
    next_backlight_btn_check = millis() + 300;
  }
#endif

  if (c != 0 && curr) {
    curr->handleInput(c);
    _auto_off = millis() + AUTO_OFF_MILLIS;   // extend auto-off timer
    _next_refresh = 100;  // trigger refresh
  }

  userLedHandler();

#ifdef PIN_BUZZER
  if (buzzer.isPlaying())  buzzer.loop();
#endif

  if (curr) curr->poll();

  if (_display != NULL && _display->isOn()) {
    if (millis() >= _next_refresh && curr) {
      _display->startFrame();
      int delay_millis = curr->render(*_display);
      if (millis() < _alert_expiry) {  // render alert popup
        _display->setTextSize(1);
        int y = _display->height() / 3;
        int p = _display->height() / 32;
        _display->setColor(DisplayDriver::DARK);
        _display->fillRect(p, y, _display->width() - p*2, y);
        _display->setColor(DisplayDriver::LIGHT);  // draw box border
        _display->drawRect(p, y, _display->width() - p*2, y);
        _display->drawTextCentered(_display->width() / 2, y + p*3, _alert);
        _next_refresh = _alert_expiry;   // will need refresh when alert is dismissed
      } else {
        _next_refresh = millis() + delay_millis;
      }
      _display->endFrame();
    }
#if AUTO_OFF_MILLIS > 0
    if (millis() > _auto_off) {
      _display->turnOff();
    }
#endif
  }

#ifdef PIN_VIBRATION
  vibration.loop();
#endif

#ifdef AUTO_SHUTDOWN_MILLIVOLTS
  if (millis() > next_batt_chck) {
    uint16_t milliVolts = getBattMilliVolts();
    if (milliVolts > 0 && milliVolts < AUTO_SHUTDOWN_MILLIVOLTS) {

      // show low battery shutdown alert
      // we should only do this for eink displays, which will persist after power loss
      #if defined(THINKNODE_M1) || defined(LILYGO_TECHO)
      if (_display != NULL) {
        _display->startFrame();
        _display->setTextSize(2);
        _display->setColor(DisplayDriver::RED);
        const int SCREEN_TOP_MARGIN = 10;
        const int SCREEN_BOTTOM_MARGIN = 10;
        int available_height = _display->height() - SCREEN_TOP_MARGIN - SCREEN_BOTTOM_MARGIN;
        int centerY = SCREEN_TOP_MARGIN + (available_height / 2);
        _display->drawTextCentered(_display->width() / 2, centerY - 10, "Low Battery.");
        _display->drawTextCentered(_display->width() / 2, centerY + 10, "Shutting Down!");
        _display->endFrame();
      }
      #endif

      shutdown();

    }
    next_batt_chck = millis() + 8000;
  }
#endif
}

char UITask::checkDisplayOn(char c) {
  if (_display != NULL) {
    if (!_display->isOn()) {
      _display->turnOn();   // turn display on and consume event
#ifdef PIN_BUZZER
      buzzer.playBoop();  // Play distinct sound when waking display
#endif
      c = 0;
    }
    _auto_off = millis() + AUTO_OFF_MILLIS;   // extend auto-off timer
    _next_refresh = 0;  // trigger refresh
  }
  return c;
}

char UITask::handleLongPress(char c) {
  if (millis() - ui_started_at < 8000) {   // long press in first 8 seconds since startup -> CLI/rescue
    the_mesh.enterCLIRescue();
    c = 0;   // consume event
  }
  // Global GPS toggle disabled - use settings menu instead
  return c;
}

char UITask::handleDoubleClick(char c) {
  MESH_DEBUG_PRINTLN("UITask: double click triggered");
  checkDisplayOn(c);
  return c;
}

char UITask::handleTripleClick(char c) {
  MESH_DEBUG_PRINTLN("UITask: triple click triggered");
  checkDisplayOn(c);
  // Buzzer toggle moved to settings menu
  c = 0;
  return c;
}

bool UITask::getGPSState() {
  if (_sensors != NULL) {
    int num = _sensors->getNumSettings();
    for (int i = 0; i < num; i++) {
      if (strcmp(_sensors->getSettingName(i), "gps") == 0) {
        return !strcmp(_sensors->getSettingValue(i), "1");
      }
    }
  } 
  return false;
}

void UITask::toggleGPS() {
    if (_sensors != NULL) {
    // toggle GPS on/off
    int num = _sensors->getNumSettings();
    for (int i = 0; i < num; i++) {
      if (strcmp(_sensors->getSettingName(i), "gps") == 0) {
        if (strcmp(_sensors->getSettingValue(i), "1") == 0) {
          _sensors->setSettingValue("gps", "0");
          notify(UIEventType::ack);
          showAlert("GPS: Disabled", 800);
        } else {
          _sensors->setSettingValue("gps", "1");
          notify(UIEventType::ack);
          showAlert("GPS: Enabled", 800);
        }
        _next_refresh = 0;
        break;
      }
    }
  }
}

void UITask::toggleBuzzer() {
    // Toggle buzzer quiet mode
  #ifdef PIN_BUZZER
    if (buzzer.isQuiet()) {
      buzzer.quiet(false);
      notify(UIEventType::ack);
      showAlert("Buzzer: ON", 800);
    } else {
      buzzer.quiet(true);
      showAlert("Buzzer: OFF", 800);
    }
    _next_refresh = 0;  // trigger refresh
  #endif
}
