#include "UITask.h"
#include "UIHelpers.h"
#include <helpers/TxtDataHelpers.h>
#include <helpers/ArduinoHelpers.h>
#include "../MyMesh.h"
#include "helpers/ui/UIStrings.h"
#include "target.h"
#include <math.h>

#ifdef NRF52_PLATFORM
#include <helpers/nrf52/NRF52Watchdog.h>
#endif

extern MyMesh the_mesh;

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
#include "SplashScreen.h"
#include "HomeScreen.h"
#include "MsgPreviewScreen.h"

// HomeScreen class moved to HomeScreen.h/cpp
// MsgPreviewScreen class moved to MsgPreviewScreen.h/cpp
// Classes removed from here - see git history if needed


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
  messages = new MessagesScreen(this, the_mesh.getMessageStore(), &rtc_clock, node_prefs);
  contacts_screen = new ContactsScreen(this, &rtc_clock, sensors, node_prefs);
  gps_screen = new GPSScreen(this, sensors, node_prefs, &rtc_clock);
  settings = new SettingsScreen(this, sensors, node_prefs);
  radio_stats = new RadioStatsScreen(this, sensors, node_prefs);
  reports_screen = new ReportsScreen(this, node_prefs);
  telemetry_screen = new TelemetryScreen(this, sensors, node_prefs);
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

void UITask::gotoAdvertPage() {
  ((HomeScreen*)home)->setPage(HomeScreen::ADVERT);
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
    c = handleLongPress(KEY_ENTER);
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
#ifdef NRF52_PLATFORM
    nrf52::resetWatchdog();  // Reset watchdog on keyboard input
#endif
    _auto_off = millis() + AUTO_OFF_MILLIS;   // extend auto-off timer
    _next_refresh = 100;  // trigger refresh
  }

  userLedHandler();

#ifdef PIN_BUZZER
  if (buzzer.isPlaying())  buzzer.loop();
#endif

  if (curr) curr->poll();

  if (_display != NULL && _display->isOn()) {
    // Check for GPS satellite count changes
    long current_gps_sats = getSatellitesCount();
    if (current_gps_sats != _cached_gps_sats) {
      _cached_gps_sats = current_gps_sats;
      _next_refresh = 100;  // trigger refresh on sat count change
    }

    if (millis() >= _next_refresh && curr) {
      _display->startFrame();
      int delay_millis = curr->render(*_display);
#ifdef NRF52_PLATFORM
      nrf52::resetWatchdog();  // Reset watchdog on UI render
#endif
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
        _display->drawTextCentered(_display->width() / 2, centerY - 10, uiGetString(_node_prefs, STR_LOW_BATTERY));
        _display->drawTextCentered(_display->width() / 2, centerY + 10, uiGetString(_node_prefs, STR_SHUTTING_DOWN));
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
        char alert_buf[40];
        if (strcmp(_sensors->getSettingValue(i), "1") == 0) {
          _sensors->setSettingValue("gps", "0");
          notify(UIEventType::ack);
          sprintf(alert_buf, "%s: %s", uiGetString(_node_prefs, STR_GPS), uiGetString(_node_prefs, STR_OFF));
          showAlert(alert_buf, 800);
        } else {
          _sensors->setSettingValue("gps", "1");
          notify(UIEventType::ack);
          sprintf(alert_buf, "%s: %s", uiGetString(_node_prefs, STR_GPS), uiGetString(_node_prefs, STR_ON));
          showAlert(alert_buf, 800);
        }
        _next_refresh = 0;
        break;
      }
    }
  }
}

long UITask::getSatellitesCount() {
  if (_sensors != NULL) {
    LocationProvider* nmea = _sensors->getLocationProvider();
    if (nmea != NULL) {
      return nmea->satellitesCount();
    }
  }
  return 0;
}

void UITask::toggleBuzzer() {
    // Toggle buzzer quiet mode
  #ifdef PIN_BUZZER
    char alert_buf[40];
    if (buzzer.isQuiet()) {
      buzzer.quiet(false);
      notify(UIEventType::ack);
      sprintf(alert_buf, "%s: %s", uiGetString(_node_prefs, STR_BUZZER), uiGetString(_node_prefs, STR_ON));
      showAlert(alert_buf, 800);
    } else {
      buzzer.quiet(true);
      sprintf(alert_buf, "%s: %s", uiGetString(_node_prefs, STR_BUZZER), uiGetString(_node_prefs, STR_OFF));
      showAlert(alert_buf, 800);
    }
    _next_refresh = 0;  // trigger refresh
  #endif
}
