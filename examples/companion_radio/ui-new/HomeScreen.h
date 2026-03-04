#pragma once

#include <helpers/ui/UIScreen.h>
#include <helpers/ui/DisplayDriver.h>
#include <MeshCore.h>
#include <CayenneLPP.h>
#include "../MyMesh.h"

// Forward declarations
class UITask;
class SensorManager;
struct NodePrefs;

namespace mesh {
  class RTCClock;
}

class HomeScreen : public UIScreen {
public:
  enum HomePage {
    FIRST,
    CONTACTS,
    ADVERT,
    MESSAGES,
    REPORTS,
    SETTINGS,
    Count    // keep as last
  };

  HomeScreen(UITask* task, mesh::RTCClock* rtc, SensorManager* sensors, NodePrefs* node_prefs);
  ~HomeScreen();

  void setPage(HomePage page);
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
  void poll() override;

private:
  UITask* _task;
  mesh::RTCClock* _rtc;
  SensorManager* _sensors;
  NodePrefs* _node_prefs;
  uint8_t _page;

  // Recent contacts buffer (UI_RECENT_LIST_SIZE is typically 4)
  static constexpr int RECENT_BUFFER_SIZE = 4;
  AdvertPath recent[RECENT_BUFFER_SIZE];

  // Screen layout constants
  static constexpr int SCREEN_TOP_MARGIN = 6;  // Header height (18px) + separator + spacing
  static constexpr int SCREEN_BOTTOM_MARGIN = 6;  // Footer height + spacing

  // Battery caching (update every 5 minutes)
  int cached_battery_percentage;
  uint16_t cached_battery_millivolts;
  unsigned long next_battery_refresh;

  // Cache computed display values
  int cached_display_width;
  int cached_display_height;
  int cached_center_x;

  // Cache for node name translation
  char cached_filtered_name[64];
  char cached_node_name_source[64];  // Track source to detect changes

  // Cache for public key hex (never changes)
  char cached_pubkey_hex[10];
  bool cached_pubkey_valid;

  // Cache for ADVERT page calculations
  int cached_line_height;
  int cached_line_spacing;

  // Helper methods
  void renderBatteryIndicator(DisplayDriver& display, uint16_t batteryMilliVolts);
  void refresh_sensors();

  // Sensor telemetry data
  CayenneLPP sensors_lpp;
  int sensors_nb;
  bool sensors_scroll;
  int sensors_scroll_offset;
  int next_sensors_refresh;
};
