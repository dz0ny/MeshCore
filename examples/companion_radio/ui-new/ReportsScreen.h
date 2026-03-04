#pragma once

#include <helpers/ui/UIScreen.h>
#include <helpers/ui/DisplayDriver.h>

class UITask;
struct NodePrefs;

class ReportsScreen : public UIScreen {
public:
  ReportsScreen(UITask* task, NodePrefs* node_prefs);

  void reset();
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
  void poll() override;

private:
  UITask* _task;
  NodePrefs* _node_prefs;
  int _selected_item;  // 0 = GPS Info, 1 = Radio Stats, 2 = Telemetry, 3 = Debug, 4 = System Stats
};
