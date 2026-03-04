#pragma once

#include <helpers/ui/UIScreen.h>
#include <helpers/ui/DisplayDriver.h>

class UITask;
class SensorManager;
struct NodePrefs;

class TelemetryScreen : public UIScreen {
public:
  TelemetryScreen(UITask* task, SensorManager* sensors, NodePrefs* node_prefs);

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
  void poll() override;

private:
  UITask* _task;
  SensorManager* _sensors;
  NodePrefs* _node_prefs;
};
