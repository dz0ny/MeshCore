#pragma once

#include <helpers/ui/UIScreen.h>
#include <helpers/ui/DisplayDriver.h>
#include <MeshCore.h>

class UITask;
class SensorManager;
struct NodePrefs;
namespace mesh {
  class RTCClock;
}

class GPSScreen : public UIScreen {
public:
  GPSScreen(UITask* task, SensorManager* sensors, NodePrefs* node_prefs, mesh::RTCClock* rtc);

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
  void poll() override;

private:
  UITask* _task;
  SensorManager* _sensors;
  NodePrefs* _node_prefs;
  mesh::RTCClock* _rtc;
};
