#pragma once

#include <helpers/ui/UIScreen.h>
#include <helpers/ui/DisplayDriver.h>
#include <MeshCore.h>

class UITask;
class SensorManager;

class GPSScreen : public UIScreen {
public:
  GPSScreen(UITask* task, SensorManager* sensors);

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
  void poll() override;

private:
  UITask* _task;
  SensorManager* _sensors;
};
