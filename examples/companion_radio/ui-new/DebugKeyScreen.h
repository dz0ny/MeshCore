#pragma once

#include <helpers/ui/UIScreen.h>
#include <helpers/ui/DisplayDriver.h>

class UITask;

class DebugKeyScreen : public UIScreen {
private:
  UITask* _task;
  static constexpr int MAX_KEY_HISTORY = 20;

  struct KeyPress {
    char key;
    unsigned long timestamp;
  };

  KeyPress _key_history[MAX_KEY_HISTORY];
  int _history_count;
  int _scroll_offset;

  const char* getKeyName(char key);

public:
  DebugKeyScreen(UITask* task);

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
  void poll() override {}
};
