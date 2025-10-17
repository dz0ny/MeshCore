#pragma once

#include <helpers/ui/UIScreen.h>
#include <helpers/ui/DisplayDriver.h>

class UITask;
class SensorManager;
class NodePrefs;

class SettingsScreen : public UIScreen {
public:
  SettingsScreen(UITask* task, SensorManager* sensors, NodePrefs* node_prefs);

  void reset();
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
  void poll() override;

private:
  UITask* _task;
  SensorManager* _sensors;
  NodePrefs* _node_prefs;

  int _selected_setting;
  int _scroll_offset;
};
