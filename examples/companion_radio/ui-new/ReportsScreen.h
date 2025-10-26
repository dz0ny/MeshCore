#pragma once

#include <helpers/ui/UIScreen.h>
#include <helpers/ui/DisplayDriver.h>

class UITask;

class ReportsScreen : public UIScreen {
public:
  ReportsScreen(UITask* task);

  void reset();
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
  void poll() override;

private:
  UITask* _task;
  int _selected_item;  // 0 = GPS Info, 1 = Radio Stats, 2 = Telemetry
};
