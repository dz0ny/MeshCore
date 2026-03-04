#pragma once

#include <helpers/ui/UIScreen.h>
#include <helpers/ui/DisplayDriver.h>
#include <MeshCore.h>

class UITask;

class SplashScreen : public UIScreen {
public:
  SplashScreen(UITask* task);

  int render(DisplayDriver& display) override;
  void poll() override;

private:
  UITask* _task;
  unsigned long dismiss_after;
  char _version_info[12];
};
