#pragma once

#include <helpers/ui/UIScreen.h>
#include <helpers/ui/DisplayDriver.h>

class UITask;
namespace mesh {
  class RTCClock;
}

class MsgPreviewScreen : public UIScreen {
public:
  MsgPreviewScreen(UITask* task, mesh::RTCClock* rtc);

  void addPreview(uint8_t path_len, const char* from_name, const char* msg);
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;

private:
  enum ViewMode {
    PREVIEW_VIEW,
    MENU_VIEW
  };

  void renderPreviewView(DisplayDriver& display);
  void renderMenuView(DisplayDriver& display);

  UITask* _task;
  mesh::RTCClock* _rtc;
  ViewMode _mode;
  int _menu_selection;

  struct MsgEntry {
    uint32_t timestamp;
    char origin[62];
    char msg[78];
  };

  static constexpr int MAX_UNREAD_MSGS = 32;
  int num_unread;
  MsgEntry unread[MAX_UNREAD_MSGS];
};
