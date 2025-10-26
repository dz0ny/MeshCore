#pragma once

#include <MeshCore.h>
#include <helpers/ui/DisplayDriver.h>
#include <helpers/ui/UIScreen.h>
#include <helpers/SensorManager.h>
#include <helpers/BaseSerialInterface.h>
#include <Arduino.h>
#include <helpers/sensors/LPPDataHelpers.h>

#ifdef PIN_BUZZER
  #include <helpers/ui/buzzer.h>
#endif
#ifdef PIN_VIBRATION
  #include <helpers/ui/GenericVibration.h>
#endif

#include "../AbstractUITask.h"
#include "../NodePrefs.h"
#include "MessagesScreen.h"
#include "DebugKeyScreen.h"
#include "SettingsScreen.h"
#include "RadioStatsScreen.h"
#include "NearbyScreen.h"
#include "GPSScreen.h"
#include "ReportsScreen.h"
#include "TelemetryScreen.h"

class UITask : public AbstractUITask {
  DisplayDriver* _display;
  SensorManager* _sensors;
#ifdef PIN_BUZZER
  genericBuzzer buzzer;
#endif
#ifdef PIN_VIBRATION
  GenericVibration vibration;
#endif
  unsigned long _next_refresh, _auto_off;
  NodePrefs* _node_prefs;
  char _alert[80];
  unsigned long _alert_expiry;
  int _msgcount;
  unsigned long ui_started_at, next_batt_chck;
  int next_backlight_btn_check = 0;
#ifdef PIN_STATUS_LED
  int led_state = 0;
  int next_led_change = 0;
  int last_led_increment = 0;
#endif

  UIScreen* splash;
  UIScreen* home;
  UIScreen* msg_preview;
  UIScreen* messages;
  UIScreen* nearby;
  UIScreen* gps_screen;
  UIScreen* debug_keys;
  UIScreen* settings;
  UIScreen* radio_stats;
  UIScreen* reports_screen;
  UIScreen* telemetry_screen;
  UIScreen* curr;

  void userLedHandler();
  
  // Button action handlers
  char checkDisplayOn(char c);
  char handleLongPress(char c);
  char handleDoubleClick(char c);
  char handleTripleClick(char c);

  void setCurrScreen(UIScreen* c);

public:

  UITask(mesh::MainBoard* board, BaseSerialInterface* serial) : AbstractUITask(board, serial), _display(NULL), _sensors(NULL) {
    next_batt_chck = _next_refresh = 0;
    ui_started_at = 0;
    curr = NULL;
  }
  void begin(DisplayDriver* display, SensorManager* sensors, NodePrefs* node_prefs);

  void gotoHomeScreen() { setCurrScreen(home); }
  void gotoMessagesHomePage();  // Jump to Messages page on home screen
  void gotoMessagesScreen() {
    ((MessagesScreen*)messages)->reset();
    setCurrScreen(messages);
  }
  void gotoNearbyScreen() {
    ((NearbyScreen*)nearby)->reset();
    setCurrScreen(nearby);
  }
  void gotoGPSScreen() {
    setCurrScreen(gps_screen);
  }
  void gotoSettingsScreen() {
    ((SettingsScreen*)settings)->reset();
    setCurrScreen(settings);
  }
  void gotoDebugKeyScreen() {
    setCurrScreen(debug_keys);
  }
  void gotoRadioStatsScreen() {
    setCurrScreen(radio_stats);
  }
  void gotoReportsScreen() {
    ((ReportsScreen*)reports_screen)->reset();
    setCurrScreen(reports_screen);
  }
  void gotoTelemetryScreen() {
    setCurrScreen(telemetry_screen);
  }
  MessagesScreen* getMessagesScreen() { return (MessagesScreen*)messages; }
  void showAlert(const char* text, int duration_millis);
  int  getMsgCount() const { return _msgcount; }
  bool hasDisplay() const { return _display != NULL; }
  bool isButtonPressed() const;

  void toggleBuzzer();
  bool getBuzzerState() {
#ifdef PIN_BUZZER
    return !buzzer.isQuiet();
#else
    return false;
#endif
  }
  bool getGPSState();
  void toggleGPS();


  // from AbstractUITask
  void msgRead(int msgcount) override;
  void newMsg(uint8_t path_len, const char* from_name, const char* text, int msgcount) override;
  void notify(UIEventType t = UIEventType::none) override;
  void loop() override;

  void shutdown(bool restart = false);
};
