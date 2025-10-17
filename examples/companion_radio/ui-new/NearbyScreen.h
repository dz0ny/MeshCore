#pragma once

#include <helpers/ui/UIScreen.h>
#include <helpers/ui/DisplayDriver.h>
#include <MeshCore.h>

class UITask;
class SensorManager;

#ifndef UI_RECENT_LIST_SIZE
  #define UI_RECENT_LIST_SIZE 4
#endif

class NearbyScreen : public UIScreen {
public:
  NearbyScreen(UITask* task, mesh::RTCClock* rtc, SensorManager* sensors);

  void reset();
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
  void poll() override;

private:
  UITask* _task;
  mesh::RTCClock* _rtc;
  SensorManager* _sensors;

  int _selected_index;  // -1 = list view, 0-3 = detail/map view
};
